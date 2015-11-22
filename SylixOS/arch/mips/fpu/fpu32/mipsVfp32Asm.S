;/**********************************************************************************************************
;**
;**                                    �й�������Դ��֯
;**
;**                                   Ƕ��ʽʵʱ����ϵͳ
;**
;**                                       SylixOS(TM)
;**
;**                               Copyright  All Rights Reserved
;**
;**--------------�ļ���Ϣ--------------------------------------------------------------------------------
;**
;** ��   ��   ��: mipsVfp32Asm.S
;**
;** ��   ��   ��: Ryan.Xin (�Ž���)
;**
;** �ļ���������: 2015 �� 11 �� 17 ��
;**
;** ��        ��: MIPS ��ϵ�ܹ� VFP32 ֧��.
;*********************************************************************************************************/

#ifndef ASSEMBLY
#define ASSEMBLY 1
#endif

#include <arch/assembler.h>

#if LW_CFG_CPU_FPU_EN > 0

    FILE_BEGIN()

    EXPORT_LABEL(mipsVfp32Sid)
    EXPORT_LABEL(mipsVfp32Enable)
    EXPORT_LABEL(mipsVfp32Disable)
    EXPORT_LABEL(mipsVfp32IsEnable)
    EXPORT_LABEL(mipsVfp32Save16)
    EXPORT_LABEL(mipsVfp32Restore16)
    EXPORT_LABEL(mipsVfp32Save32)
    EXPORT_LABEL(mipsVfp32Restore32)

;/*********************************************************************************************************
;  ����������������������(mipsVfp32Sid �����ȱ���)
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Sid)
    LA      V0, 0
    JR      RA
    NOP
FUNC_END(mipsVfp32Sid)

FUNC_DEF(mipsVfp32Enable)
    MFC0(T0, CP0_STATUS)
    OR      T0, M_StatusCU1
    MTC0(T0, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(mipsVfp32Enable)

FUNC_DEF(mipsVfp32Disable)
    MFC0(T0, CP0_STATUS)
    AND     T0, ~M_StatusCU1
    MTC0(T0, CP0_STATUS)
    JR      RA
    NOP
FUNC_END(mipsVfp32Disable)

FUNC_DEF(mipsVfp32IsEnable)
    MOV     V0, ZERO                                                    ;/*  OK return value             */
    MFC0(T0, CP0_STATUS)
    AND     T0, M_StatusCU1                                             ;/*  coprocessor 1 enabled       */
    BEQ     T0, ZERO, 2f
    NOP
    LI      V0, 1
2:
    JR      RA
    NOP
FUNC_END(mipsVfp32IsEnable)

;/*********************************************************************************************************
;  VFP-D16
;   +-----------+
;   | freg[31]  |    + 0x80
;   |  ...      |
;   | freg[2]   |    + 0x0C
;   | freg[1]   |    + 0x08
;   | freg[0]   |    + 0x04
;   | fpscr     | <-- cpu_fpu_context ( = a0 )
;   +-----------+
;*********************************************************************************************************/

;/*********************************************************************************************************
;  MIPSVFP ����/�ָ��Ĵ��� 32Bit MIPS ʹ��ż���ļĴ���
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Save16)
    MFC0(T0, CP0_STATUS)
    OR      T0, M_StatusCU1
    MTC0(T0, CP0_STATUS)
    CFC1(T0,  CP1_STATUS)
    SW      T0,     0x00(A0)
    SWC1    FP0,    0x04(A0)
    SWC1    FP2,    0x0c(A0)
    SWC1    FP4,    0x14(A0)
    SWC1    FP6,    0x1c(A0)
    SWC1    FP8,    0x24(A0)
    SWC1    FP10,   0x2c(A0)
    SWC1    FP12,   0x34(A0)
    SWC1    FP14,   0x3c(A0)
    SWC1    FP16,   0x44(A0)
    SWC1    FP18,   0x4c(A0)
    SWC1    FP20,   0x54(A0)
    SWC1    FP22,   0x5c(A0)
    SWC1    FP24,   0x64(A0)
    SWC1    FP26,   0x6c(A0)
    SWC1    FP28,   0x74(A0)
    SWC1    FP30,   0x7c(A0)
    JR      RA
    LI      V0,     0
FUNC_END(mipsVfp32Save16)

FUNC_DEF(mipsVfp32Restore16)
    MFC0(T0, CP0_STATUS)
    OR      T0, M_StatusCU1
    MTC0(T0, CP0_STATUS)
    LW      T0,     0X00(A0)
    LWC1    FP0,    0x04(A0)
    LWC1    FP2,    0x0c(A0)
    LWC1    FP4,    0x14(A0)
    LWC1    FP6,    0x1c(A0)
    LWC1    FP8,    0x24(A0)
    LWC1    FP10,   0x2c(A0)
    LWC1    FP12,   0x34(A0)
    LWC1    FP14,   0x3c(A0)
    LWC1    FP16,   0x44(A0)
    LWC1    FP18,   0x4c(A0)
    LWC1    FP20,   0x54(A0)
    LWC1    FP22,   0x5c(A0)
    LWC1    FP24,   0x64(A0)
    LWC1    FP26,   0x6c(A0)
    LWC1    FP28,   0x74(A0)
    LWC1    FP30,   0x7c(A0)
    CTC1(T0,     CP1_STATUS)                                            ;/*  restore fpp status reg      */
    JR      RA
    LI      V0,     0
FUNC_END(mipsVfp32Restore16)

;/*********************************************************************************************************
;  MIPSVFP32 ����/�ָ��Ĵ��� 64Bit MIPS ʹ��32�ļĴ���
;*********************************************************************************************************/

FUNC_DEF(mipsVfp32Save32)
    JR      RA
    NOP
FUNC_END(mipsVfp32Save32)

FUNC_DEF(mipsVfp32Restore32)
    JR      RA
    NOP
FUNC_END(mipsVfp32Restore32)

    FILE_END()

#endif
;/*********************************************************************************************************
;  END
;*********************************************************************************************************/