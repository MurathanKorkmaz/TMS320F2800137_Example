
#include "f280013x_device.h"         // F2806x Headerfile
#include "f280013x_examples.h"       // F2806x Examples Headerfile
#include "f280013x_epwm_defines.h"   // useful defines for initialization


void HRPWM2_Config(Uint16);
void InitEPwm2Gpio(void);

Uint16 i, j, DutyFine1, DutyFine2, n, update, DutyCycle;

/**
 * main.c
 */
int main(void)
{

    InitSysCtrl();

    InitEPwm2Gpio();

    DINT;

    InitPieCtrl();

    IER = 0x0000;
    IFR = 0x0000;

    InitPieVectTable();

    update = 1;
    DutyFine1 = 70;
    DutyFine2 = 255;
    DutyCycle = 0;

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    HRPWM2_Config(10);      // ePWM1 target, Period = 10

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;

    while (update ==1)
    {
        DutyCycle = 50;
        EPwm2Regs.CMPA.bit.CMPAHR = DutyCycle << 8;
    }

	return 0;
}


void HRPWM2_Config(Uint16 period)
{
    //
    // ePWM2 register configuration with HRPWM
    // ePWM2A toggle low/high with MEP control on Rising edge
    //
    EPwm2Regs.TBCTL.bit.PRDLD = TB_IMMEDIATE;    // set Immediate load
    EPwm2Regs.TBPRD = period-1;                  // PWM frequency = 1 / period
    EPwm2Regs.CMPA.bit.CMPA = period / 2;       // set duty 50% initially
    EPwm2Regs.CMPA.bit.CMPAHR = (1 << 8);       // initialize HRPWM extension
    EPwm2Regs.CMPB.bit.CMPB = period / 2;                 // set duty 50% initially
    EPwm2Regs.TBPHS.all = 0;
    EPwm2Regs.TBCTR = 0;

    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;
    EPwm2Regs.TBCTL.bit.PHSEN = TB_DISABLE;      // EPwm2 is the Master
    EPwm2Regs.EPWMSYNCINSEL.all = SYNC_IN_SRC_DISABLE_ALL;
    EPwm2Regs.EPWMSYNCOUTEN.all = SYNC_OUT_SRC_DISABLE_ALL;
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;
    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;

    EPwm2Regs.AQCTLA.bit.ZRO = AQ_CLEAR;        // PWM toggle low/high
    EPwm2Regs.AQCTLA.bit.CAU = AQ_SET;
    EPwm2Regs.AQCTLB.bit.ZRO = AQ_CLEAR;
    EPwm2Regs.AQCTLB.bit.CBU = AQ_SET;

    EALLOW;
    EPwm2Regs.HRCNFG.all = 0x0;
    EPwm2Regs.HRCNFG.bit.EDGMODE = HR_REP;      // MEP control on Rising edge
    EPwm2Regs.HRCNFG.bit.CTLMODE = HR_CMP;
    EPwm2Regs.HRCNFG.bit.HRLOAD  = HR_CTR_ZERO;
    EDIS;
}
