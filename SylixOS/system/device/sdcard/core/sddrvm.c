/*********************************************************************************************************
**
**                                    �й�������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: sdm.h
**
** ��   ��   ��: Zeng.Bo (����)
**
** �ļ���������: 2014 �� 10 �� 24 ��
**
** ��        ��: sd drv manager layer

** BUG:
2014.11.07  ���ӹ¶��豸����,�������Ȳ����豸,��ע��������,�豸�ܹ���ȷ����.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "../SylixOS/kernel/include/k_kernel.h"
#include "../SylixOS/system/include/s_system.h"
/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if (LW_CFG_DEVICE_EN > 0) && (LW_CFG_SDCARD_EN > 0)
#include "sddrvm.h"
#include "sdutil.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __SDM_DEVNAME_MAX               32
/*********************************************************************************************************
  �� SDM_SDIO ģ��ɼ����¼�
*********************************************************************************************************/
#define SDM_EVENT_NEW_DRV               3
/*********************************************************************************************************
  SDM �ڲ�ʹ�õ����ݽṹ
*********************************************************************************************************/
struct __sdm_sd_dev;
struct __sdm_host;
struct __sdm_host_drv_funcs;
struct __sdm_host_chan;

typedef struct __sdm_sd_dev            __SDM_SD_DEV;
typedef struct __sdm_host              __SDM_HOST;
typedef struct __sdm_host_drv_funcs    __SDM_HOST_DRV_FUNCS;
typedef struct __sdm_host_chan         __SDM_HOST_CHAN;

struct __sdm_sd_dev {
    SD_DRV           *SDMDEV_psddrv;
    VOID             *SDMDEV_pvDevPriv;
    CHAR              SDMDEV_pcDevName[__SDM_DEVNAME_MAX];
    INT               SDMDEV_iUnit;
    PLW_SDCORE_DEVICE SDMDEV_psdcoredev;
};

struct __sdm_host_chan {
    __SDM_HOST_DRV_FUNCS    *SDMHOSTCHAAN_pdrvfuncs;
};

struct __sdm_host_drv_funcs {
    INT     (*SDMHOSTDRV_pfuncCallbackInstall)
            (
             __SDM_HOST_CHAN  *psdmhostchan,
             INT               iCallbackType,
             SD_CALLBACK       callback,
             PVOID             pvCallbackArg
            );
    VOID    (*SDMHOSTDRV_pfuncSpicsEn)(__SDM_HOST_CHAN *psdmhostchan);
    VOID    (*SDMHOSTDRV_pfuncSpicsDis)(__SDM_HOST_CHAN *psdmhostchan);
};

struct __sdm_host {
    LW_LIST_LINE     SDMHOST_lineManage;
    __SDM_HOST_CHAN  SDMHOST_sdmhostchan;
    SD_HOST         *SDMHOST_psdhost;
    __SDM_SD_DEV    *SDMHOST_psdmdevAttached;
    BOOL             SDMHOST_bDevIsOrphan;
};
/*********************************************************************************************************
  ǰ������
*********************************************************************************************************/
static __SDM_HOST *__sdmHostFind(CPCHAR cpcName);
static VOID        __sdmHostInsert(__SDM_HOST *psdmhost);
static VOID        __sdmHostDelete(__SDM_HOST *psdmhost);
static VOID        __sdmDrvInsert(SD_DRV *psddrv);
static VOID        __sdmDrvDelete(SD_DRV *psddrv);


static VOID        __sdmDevCreate(__SDM_HOST *psdmhost);
static VOID        __sdmDevDelete(__SDM_HOST *psdmhost);
static VOID        __sdmSdioIntHandle(__SDM_HOST *psdmhost);

static INT         __sdmCallbackInstall(__SDM_HOST_CHAN  *psdmhostchan,
                                        INT               iCallbackType,
                                        SD_CALLBACK       callback,
                                        PVOID             pvCallbackArg);
static INT         __sdmCallbackUnInstall(SD_HOST *psdhost);

static VOID        __sdmSpiCsEn(__SDM_HOST_CHAN *psdmhostchan);
static VOID        __sdmSpiCsDis(__SDM_HOST_CHAN *psdmhostchan);
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
static LW_SPINLOCK_DEFINE       (_G_slSdmHostLock);

static LW_LIST_LINE_HEADER       _G_plineSddrvHeader   = LW_NULL;
static LW_LIST_LINE_HEADER       _G_plineSdmhostHeader = LW_NULL;

#define __SDM_HOST_LOCK()        LW_SPIN_LOCK_IRQ(&_G_slSdmHostLock, &iregInterLevel)
#define __SDM_HOST_UNLOCK()      LW_SPIN_UNLOCK_IRQ(&_G_slSdmHostLock, iregInterLevel)

static __SDM_HOST_DRV_FUNCS      _G_sdmhostdrvfuncs;
/*********************************************************************************************************
** ��������: API_SdmLibInit
** ��������: SDM ������ʼ��
** ��    ��: NONE
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmLibInit (VOID)
{
    _G_sdmhostdrvfuncs.SDMHOSTDRV_pfuncCallbackInstall = __sdmCallbackInstall;
    _G_sdmhostdrvfuncs.SDMHOSTDRV_pfuncSpicsDis        = __sdmSpiCsDis;
    _G_sdmhostdrvfuncs.SDMHOSTDRV_pfuncSpicsEn         = __sdmSpiCsEn;

    API_SdLibInit();

    LW_SPIN_INIT(&_G_slSdmHostLock);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmHostRegister
** ��������: �� SDM ע��һ�� HOST ��Ϣ
** ��    ��: pHost        Host ��Ϣ, ע�� SDM �ڲ���ֱ�����øö���, ��˸ö�����Ҫ������Ч
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmHostRegister (SD_HOST *psdhost)
{
    __SDM_HOST *psdmhost;

    if (!psdhost                              ||
        !psdhost->SDHOST_cpcName              ||
        !psdhost->SDHOST_pfuncCallbackInstall ||
        !psdhost->SDHOST_pfuncCallbackUnInstall) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "host must provide: name, callback install"
                                           " method and callback uninstall method.\r\n");
        return (PX_ERROR);
    }

    if (psdhost->SDHOST_iType == SDHOST_TYPE_SPI) {
        if (!psdhost->SDHOST_pfuncSpicsDis ||
            !psdhost->SDHOST_pfuncSpicsEn) {
            _DebugHandle(__ERRORMESSAGE_LEVEL, "spi type host must provide:"
                                               " spi chip select method.\r\n");
            return (PX_ERROR);
        }
    }

    /*
     * SDM�ڲ���HOST�����ƺ�����, ������������ͬ������
     */
    psdmhost = __sdmHostFind(psdhost->SDHOST_cpcName);
    if (psdmhost) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is already a same name host registered.\r\n");
        return  (PX_ERROR);
    }

    psdmhost = (__SDM_HOST *)__SHEAP_ALLOC(sizeof(__SDM_HOST));
    if(!psdmhost) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system memory low.\r\n");
        return  (PX_ERROR);
    }

    psdmhost->SDMHOST_psdhost         = psdhost;
    psdmhost->SDMHOST_psdmdevAttached = NULL;
    psdmhost->SDMHOST_bDevIsOrphan    = FALSE;

    psdmhost->SDMHOST_sdmhostchan.SDMHOSTCHAAN_pdrvfuncs = &_G_sdmhostdrvfuncs;

    __sdmHostInsert(psdmhost);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmHostUnRegister
** ��������: �� SDM ע��һ�� HOST ��Ϣ
** ��    ��: cpcHostName      ���ص�����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmHostUnRegister (CPCHAR cpcHostName)
{
    __SDM_HOST *psdmhost;

    if (!cpcHostName) {
        return (PX_ERROR);
    }

    psdmhost = __sdmHostFind(cpcHostName);
    if (!psdmhost) {
        return  (PX_ERROR);
    }

    if (psdmhost->SDMHOST_psdmdevAttached) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "there is always a device attached.\r\n");
        return  (PX_ERROR);
    }

    if (psdmhost->SDMHOST_bDevIsOrphan) {
        _DebugHandle(__LOGMESSAGE_LEVEL, "warning: there is always a orphan device attached.\r\n");
    }

    __sdmHostDelete(psdmhost);

    __SHEAP_FREE(psdmhost);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmHostCapGet
** ��������: ���ݺ��Ĵ�������ö�Ӧ�������Ĺ�����Ϣ
** ��    ��: psdcoredev       �����豸�������
**           piCapbility      ���淵�صĿ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT  API_SdmHostCapGet (PLW_SDCORE_DEVICE psdcoredev, INT *piCapbility)
{
    CPCHAR      cpcHostName;
    __SDM_HOST *psdmhost;

    cpcHostName = API_SdCoreDevAdapterName(psdcoredev);
    if (!cpcHostName) {
        return  (PX_ERROR);
    }

    psdmhost = __sdmHostFind(cpcHostName);
    if (!psdmhost) {
        return  (PX_ERROR);
    }

    *piCapbility = psdmhost->SDMHOST_psdhost->SDHOST_iCapbility;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmHostInterEn
** ��������: ʹ�ܿ������� SDIO �ж�
** ��    ��: psdcoredev       �����豸�������
**           bEnable          �Ƿ�ʹ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API VOID  API_SdmHostInterEn (PLW_SDCORE_DEVICE psdcoredev, BOOL bEnable)
{
    CPCHAR      cpcHostName;
    __SDM_HOST *psdmhost;
    SD_HOST    *psdhost;

    cpcHostName = API_SdCoreDevAdapterName(psdcoredev);
    if (!cpcHostName) {
        return;
    }

    psdmhost = __sdmHostFind(cpcHostName);
    if (!psdmhost) {
        return;
    }

    psdhost = psdmhost->SDMHOST_psdhost;
    if (psdhost->SDHOST_pfuncSdioIntEn) {
        psdhost->SDHOST_pfuncSdioIntEn(psdhost, bEnable);
    }
}
/*********************************************************************************************************
** ��������: API_SdmSdDrvRegister
** ��������: ��SDMע��һ���豸Ӧ������
** ��    ��: psddrv       sd ����. ע�� SDM �ڲ���ֱ�����øö���, ��˸ö�����Ҫ������Ч
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmSdDrvRegister (SD_DRV *psddrv)
{
    if (!psddrv) {
        return  (PX_ERROR);
    }

    psddrv->SDDRV_pvSpec = __sdUnitPoolCreate();
    if (!psddrv->SDDRV_pvSpec) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system memory low.\r\n");
        return  (PX_ERROR);
    }

    API_AtomicSet(0, &psddrv->SDDRV_atomicDevCnt);

    __sdmDrvInsert(psddrv);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmSdDrvUnRegister
** ��������: �� SDM ע��һ���豸Ӧ������
** ��    ��: psddrv       sd ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API INT   API_SdmSdDrvUnRegister (SD_DRV *psddrv)
{
    if (!psddrv) {
        return  (PX_ERROR);
    }

    if (API_AtomicGet(&psddrv->SDDRV_atomicDevCnt)) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "exist device using this drv.\r\n");
        return  (PX_ERROR);
    }

    __sdUnitPoolDelete(psddrv->SDDRV_pvSpec);

    __sdmDrvDelete(psddrv);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: API_SdmEventNotify
** ��������: �� SDM ֪ͨ�¼�
** ��    ��: cpcHostName      ����������
**           iEvtType         �¼�����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
LW_API VOID  API_SdmEventNotify (CPCHAR cpcHostName, INT iEvtType)
{
    INTREG             iregInterLevel;
    PLW_LIST_LINE      plineTemp;
    __SDM_HOST        *psdmhost = LW_NULL;

    /*
     * ��ע��һ���µ�������, �������� Host �ϵĹ¶��豸
     */
    if ((iEvtType == SDM_EVENT_NEW_DRV) && !cpcHostName) {
        __SDM_HOST_LOCK();
        for (plineTemp  = _G_plineSdmhostHeader;
             plineTemp != LW_NULL;
             plineTemp  = _list_line_get_next(plineTemp)) {

            psdmhost = _LIST_ENTRY(plineTemp, __SDM_HOST, SDMHOST_lineManage);
            if (!psdmhost->SDMHOST_psdmdevAttached &&
                 psdmhost->SDMHOST_bDevIsOrphan) {
                hotplugEvent((VOIDFUNCPTR)__sdmDevCreate, (PVOID)psdmhost, 0, 0, 0, 0, 0);
            }
        }
        __SDM_HOST_UNLOCK();

        return;
    }

    /*
     * ����Host�������¼�
     */
    psdmhost = __sdmHostFind(cpcHostName);
    if (!psdmhost) {
        return;
    }

    switch (iEvtType) {
    case SDM_EVENT_DEV_INSERT:
        hotplugEvent((VOIDFUNCPTR)__sdmDevCreate, (PVOID)psdmhost, 0, 0, 0, 0, 0);
        break;

    case SDM_EVENT_DEV_REMOVE:
        hotplugEvent((VOIDFUNCPTR)__sdmDevDelete, (PVOID)psdmhost, 0, 0, 0, 0, 0);
        break;

    case SDM_EVENT_SDIO_INTERRUPT:
        __sdmSdioIntHandle(psdmhost);
        break;

    default:
        break;
    }
}
/*********************************************************************************************************
** ��������: __sdmDevCreate
** ��������: SDM �豸��������
** ��    ��: psdmhost     SDM �����Ŀ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdmDevCreate (__SDM_HOST *psdmhost)
{
    SD_DRV            *psddrv;
    __SDM_SD_DEV      *psdmdev;
    PLW_SDCORE_DEVICE  psdcoredev;
    PLW_LIST_LINE      plineTemp;
    SD_HOST           *psdhost = psdmhost->SDMHOST_psdhost;

    INT                iRet;

    psdmdev = (__SDM_SD_DEV *)__SHEAP_ALLOC(sizeof(__SDM_SD_DEV));
    if (!psdmdev) {
        _DebugHandle(__ERRORMESSAGE_LEVEL, "system memory low.\r\n");
        return;
    }

    for (plineTemp  = _G_plineSddrvHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        psddrv = _LIST_ENTRY(plineTemp, SD_DRV, SDDRV_lineManage);

        iRet = __sdUnitGet(psddrv->SDDRV_pvSpec);
        if (iRet < 0) {
            goto __err1;
        }

        psdmdev->SDMDEV_iUnit = iRet;
        snprintf(psdmdev->SDMDEV_pcDevName, __SDM_DEVNAME_MAX, "%s%d", psddrv->SDDRV_cpcName, iRet);

        psdcoredev = API_SdCoreDevCreate(psdhost->SDHOST_iType,
                                         psdhost->SDHOST_cpcName,
                                         psdmdev->SDMDEV_pcDevName,
                                         (PLW_SDCORE_CHAN)&psdmhost->SDMHOST_sdmhostchan);
        if (!psdcoredev) {
            goto __err2;
        }

        /*
         * ��Ϊ�ڽ������ľ��������豸���������п��ܻ����HOST�¼�֪ͨ
         * ������ǰ��ɴ����ж�ʱ��Ҫ������
         */
        psdmdev->SDMDEV_psddrv            = psddrv;
        psdmdev->SDMDEV_psdcoredev        = psdcoredev;
        psdmhost->SDMHOST_psdmdevAttached = psdmdev;
        psdmhost->SDMHOST_bDevIsOrphan    = FALSE;

        iRet = psddrv->SDDRV_pfuncDevCreate(psddrv, psdcoredev, &psdmdev->SDMDEV_pvDevPriv);
        if (iRet != ERROR_NONE) {
            /*
             * ��ɾ�� core dev ֮ǰһ��Ҫ��ж�ذ�װ�Ļص�����
             * �����豸�����ͷź�, host �˻��п���ȥ���ð�װ�Ļص�����
             */
            __sdmCallbackUnInstall(psdmhost->SDMHOST_psdhost);
            API_SdCoreDevDelete(psdcoredev);
            __sdUnitPut(psddrv->SDDRV_pvSpec, psdmdev->SDMDEV_iUnit);

        } else {
            if (psdhost->SDHOST_pfuncDevAttach) {
                psdhost->SDHOST_pfuncDevAttach(psdmhost->SDMHOST_psdhost, psdmdev->SDMDEV_pcDevName);
            }

            API_AtomicInc(&psddrv->SDDRV_atomicDevCnt);

            return;
        }
    }

__err2:
    if (psddrv) {
        __sdUnitPut(psddrv->SDDRV_pvSpec, psdmdev->SDMDEV_iUnit);
    }

__err1:
    __SHEAP_FREE(psdmdev);

    /*
     * ��ʾ�豸�Ѿ�����, ����û�о���������ɹ������豸����
     */
    psdmhost->SDMHOST_psdmdevAttached = NULL;
    psdmhost->SDMHOST_bDevIsOrphan    = TRUE;
}
/*********************************************************************************************************
** ��������: __sdmDevDelete
** ��������: SDM �豸ɾ������
** ��    ��: psdmhost     SDM �����Ŀ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdmDevDelete (__SDM_HOST *psdmhost)
{
    SD_DRV          *psddrv;
    __SDM_SD_DEV    *psdmdev;

    psdmdev = psdmhost->SDMHOST_psdmdevAttached;
    if (!psdmdev) {
        psdmhost->SDMHOST_bDevIsOrphan = FALSE;
        return;
    }

    psddrv = psdmdev->SDMDEV_psddrv;
    psddrv->SDDRV_pfuncDevDelete(psddrv, psdmdev->SDMDEV_pvDevPriv);

    __sdmCallbackUnInstall(psdmhost->SDMHOST_psdhost);
    API_SdCoreDevDelete(psdmdev->SDMDEV_psdcoredev);

    __sdUnitPut(psddrv->SDDRV_pvSpec, psdmdev->SDMDEV_iUnit);
    __SHEAP_FREE(psdmdev);

    psdmhost->SDMHOST_psdmdevAttached = NULL;
    psdmhost->SDMHOST_bDevIsOrphan    = FALSE;

    if (psdmhost->SDMHOST_psdhost->SDHOST_pfuncDevDetach) {
        psdmhost->SDMHOST_psdhost->SDHOST_pfuncDevDetach(psdmhost->SDMHOST_psdhost);
    }

    API_AtomicDec(&psddrv->SDDRV_atomicDevCnt);
}
/*********************************************************************************************************
** ��������: __sdmSdioIntHandle
** ��������: SDM SDIO�жϴ���
** ��    ��: psdmhost     SDM �����Ŀ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdmSdioIntHandle (__SDM_HOST *psdmhost)
{
    /*
     * �����֧��SDIO����ֻ��ɾ�����´��뼴��
     */
#if (LW_CFG_SDCARD_SDIO_EN > 0)

    __SDM_SD_DEV    *psdmdev;

    INT  __sdiobaseDevIrqHandle(SD_DRV *psddrv,  VOID *pvDevPriv);

    psdmdev = psdmhost->SDMHOST_psdmdevAttached;
    if (!psdmdev) {
        return;
    }

    __sdiobaseDevIrqHandle(psdmdev->SDMDEV_psddrv, psdmdev->SDMDEV_pvDevPriv);
#endif
}
/*********************************************************************************************************
** ��������: __sdmHostFind
** ��������: ���� SDM �����Ŀ�����
** ��    ��: cpcName      ����������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static __SDM_HOST *__sdmHostFind (CPCHAR cpcName)
{
    REGISTER PLW_LIST_LINE      plineTemp;
    REGISTER __SDM_HOST        *psdmhost = LW_NULL;
    INTREG                      iregInterLevel;

    if (cpcName == LW_NULL) {
        return  (LW_NULL);
    }

    __SDM_HOST_LOCK();
    for (plineTemp  = _G_plineSdmhostHeader;
         plineTemp != LW_NULL;
         plineTemp  = _list_line_get_next(plineTemp)) {

        psdmhost = _LIST_ENTRY(plineTemp, __SDM_HOST, SDMHOST_lineManage);
        if (lib_strcmp(cpcName, psdmhost->SDMHOST_psdhost->SDHOST_cpcName) == 0) {
            break;
        }
    }
    __SDM_HOST_UNLOCK();

    if (plineTemp) {
        return  (psdmhost);
    } else {
        return  (LW_NULL);
    }
}
/*********************************************************************************************************
** ��������: __sdmHostInsert
** ��������: �� SDM �����Ŀ�������������һ������������
** ��    ��: psdmhost     SDM �����Ŀ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmHostInsert (__SDM_HOST *psdmhost)
{
    INTREG   iregInterLevel;

    __SDM_HOST_LOCK();
    _List_Line_Add_Ahead(&psdmhost->SDMHOST_lineManage, &_G_plineSdmhostHeader);
    __SDM_HOST_UNLOCK();
}
/*********************************************************************************************************
** ��������: __sdmHostDelete
** ��������: �� SDM �����Ŀ���������ɾ��һ������������
** ��    ��: psdmhost     SDM �����Ŀ���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmHostDelete (__SDM_HOST *psdmhost)
{
    INTREG   iregInterLevel;

    __SDM_HOST_LOCK();
    _List_Line_Del(&psdmhost->SDMHOST_lineManage, &_G_plineSdmhostHeader);
    __SDM_HOST_UNLOCK();
}
/*********************************************************************************************************
** ��������: __sdmDrvInsert
** ��������: �� SDM ������ SD ������������һ����������
** ��    ��: psddrv     SDM ��������������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmDrvInsert (SD_DRV *psddrv)
{
    _List_Line_Add_Ahead(&psddrv->SDDRV_lineManage, &_G_plineSddrvHeader);
}
/*********************************************************************************************************
** ��������: __sdmDrvDelete
** ��������: �� SDM ������ SD ��������ɾ��һ������������
** ��    ��: psddrv     SDM ��������������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __sdmDrvDelete (SD_DRV *psddrv)
{
    _List_Line_Del(&psddrv->SDDRV_lineManage, &_G_plineSddrvHeader);
}
/*********************************************************************************************************
** ��������: __sdmCallbackInstall
** ��������: SDM �ڲ��豸�ص�������װ ����.
** ��    ��: psdmhostchan     SDM �ڲ�������ͨ����������
**           iCallbackType    �ص���������
**           callback         ��װ�Ļص�����
**           pvCallbackArg    �ص������Ĳ���
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdmCallbackInstall (__SDM_HOST_CHAN  *psdmhostchan,
                                 INT               iCallbackType,
                                 SD_CALLBACK       callback,
                                 PVOID             pvCallbackArg)
{
    __SDM_HOST *psdmhost;
    SD_HOST    *psdhost;

    INT         iRet = PX_ERROR;

    psdmhost = __CONTAINER_OF(psdmhostchan, __SDM_HOST, SDMHOST_sdmhostchan);
    psdhost  = psdmhost->SDMHOST_psdhost;

    if (psdhost->SDHOST_pfuncCallbackInstall) {
        iRet = psdhost->SDHOST_pfuncCallbackInstall(psdhost, iCallbackType, callback, pvCallbackArg);
    }

    return  (iRet);
}
/*********************************************************************************************************
** ��������: __sdmCallbackUnInstall
** ��������: SDM �ڲ���ָ����������ж�ذ�װ�Ļص����� ����
** ��    ��: psdhost      ������ע�����Ϣ�ṹ����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __sdmCallbackUnInstall (SD_HOST *psdhost)
{
    if (psdhost->SDHOST_pfuncCallbackUnInstall) {
        psdhost->SDHOST_pfuncCallbackUnInstall(psdhost, SDHOST_CALLBACK_CHECK_DEV);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdmSpiCsEn
** ��������: SDM �ڲ�ʹ�õ� SPI ������Ƭѡ����
** ��    ��: psdmhostchan     ������ͨ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdmSpiCsEn (__SDM_HOST_CHAN *psdmhostchan)
{
    __SDM_HOST *psdmhost;
    SD_HOST    *psdhost;

    psdmhost = __CONTAINER_OF(psdmhostchan, __SDM_HOST, SDMHOST_sdmhostchan);
    psdhost  = psdmhost->SDMHOST_psdhost;

    if (psdhost->SDHOST_pfuncSpicsEn) {
        psdhost->SDHOST_pfuncSpicsEn(psdhost);
    }
}
/*********************************************************************************************************
** ��������: __sdmSpiCsDis
** ��������: SDM �ڲ�ʹ�õ� SPI ������Ƭѡ����
** ��    ��: psdmhostchan     ������ͨ��
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID __sdmSpiCsDis (__SDM_HOST_CHAN *psdmhostchan)
{
    __SDM_HOST *psdmhost;
    SD_HOST    *psdhost;

    psdmhost = __CONTAINER_OF(psdmhostchan, __SDM_HOST, SDMHOST_sdmhostchan);
    psdhost  = psdmhost->SDMHOST_psdhost;

    if (psdhost->SDHOST_pfuncSpicsDis) {
        psdhost->SDHOST_pfuncSpicsDis(psdhost);
    }
}
#endif                                                                  /*  (LW_CFG_DEVICE_EN > 0)      */
                                                                        /*  (LW_CFG_SDCARD_EN > 0)      */
/*********************************************************************************************************
  END
*********************************************************************************************************/