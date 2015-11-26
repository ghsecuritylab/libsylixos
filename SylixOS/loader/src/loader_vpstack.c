/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: loader_vpstack.c
**
** 创   建   人: Han.hui (韩辉)
**
** 文件创建日期: 2015 年 11 月 25 日
**
** 描        述: 进程内线程堆栈内存管理.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
/*********************************************************************************************************
  裁剪支持
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0
#include "../include/loader_lib.h"
/*********************************************************************************************************
** 函数名称: vprocStackAlloc
** 功能描述: 分配 stack
** 输　入  : ptcbNew       新建任务
**           ulOption      任务创建选项
**           stSize        堆栈大小
** 输　出  : 堆栈
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
PVOID  vprocStackAlloc (PLW_CLASS_TCB  ptcbNew, ULONG  ulOption, size_t  stSize)
{
    PVOID         pvRet;

#if LW_CFG_VMM_EN > 0
    PLW_CLASS_TCB    ptcbCur;
    LW_LD_VPROC     *pvproc;
    
    LW_TCB_GET_CUR_SAFE(ptcbCur);
    
    pvproc = __LW_VP_GET_TCB_PROC(ptcbCur);
    if (pvproc && !(ulOption & LW_OPTION_OBJECT_GLOBAL)) {
        ptcbNew->TCB_iStkLocation = LW_TCB_STK_VP;
        _BugHandle(!pvproc->VP_pfuncMalloc, LW_TRUE, "vproc no malloc function!\r\n");

        LW_SOFUNC_PREPARE(pvproc->VP_pfuncMalloc);
        pvRet = pvproc->VP_pfuncMalloc(stSize);
        
    } else if (ulOption & LW_OPTION_THREAD_STK_MAIN) {                  /*  主线程堆栈                  */
        ptcbNew->TCB_iStkLocation = LW_TCB_STK_MAIN;
        pvRet = API_VmmMalloc(stSize);
        
    } else 
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
    {
        ptcbNew->TCB_iStkLocation = LW_TCB_STK_HEAP;
        pvRet = __KHEAP_ALLOC(stSize);
    }
    
    return  (pvRet);
}
/*********************************************************************************************************
** 函数名称: vprocStackFree
** 功能描述: 释放 stack
** 输　入  : ptcbDel       被删除的任务
**           pvStack       线程堆栈
**           bImmed        立即回收
** 输　出  : 堆栈
** 全局变量: 
** 调用模块: 
*********************************************************************************************************/
VOID  vprocStackFree (PLW_CLASS_TCB  ptcbDel, PVOID  pvStack, BOOL  bImmed)
{
#if LW_CFG_VMM_EN > 0
    LW_LD_VPROC     *pvproc;

    switch (ptcbDel->TCB_iStkLocation) {
    
    case LW_TCB_STK_MAIN:                                               /*  主线程由 vprocDestroy 回收  */
        pvproc = __LW_VP_GET_TCB_PROC(ptcbDel);
        if ((pvproc == LW_NULL) || bImmed) {
            API_VmmFree(pvStack);
            if (pvproc) {
                pvproc->VP_pvMainStack = LW_NULL;
            }
        }
        break;
        
    case LW_TCB_STK_VP:                                                 /*  进程内存堆                  */
        pvproc = __LW_VP_GET_TCB_PROC(ptcbDel);
        if (pvproc && pvproc->VP_pfuncFree) {
            LW_SOFUNC_PREPARE(pvproc->VP_pfuncFree);
            pvproc->VP_pfuncFree(pvStack);
        }
        break;
        
    case LW_TCB_STK_HEAP:
        __KHEAP_FREE(pvStack);
        break;
        
    default:
        _BugHandle(LW_TRUE, LW_FALSE, "unknown stack property!\r\n");
        break;
    }
#else
    __KHEAP_FREE(pvStack);
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
}

#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
