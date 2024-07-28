
#include "f28x_project.h"


// defines function EPWM2 send signal
void HRPWM2_Config(Uint16);
void InitEPwm2Gpio(void);

// defines function ADC read signal
void initADC(void);
void initEPWM(void);
void initADCSOC(void);
__interrupt void adcA1ISR(void);
long map(long x, long in_min, long in_max, long out_min, long out_max);


// Global variables ADC
uint16_t Dutycycle1 = 0,Dutycycle2 = 0, Voltage1 = 0, Voltage2 = 0;

void main(void)
{
    InitSysCtrl();

    // EPWM için
    InitEPwm2Gpio();

    // ADC için
    InitGpio();

    DINT;

    InitPieCtrl();

    IER = 0x0000;
    IFR = 0x0000;

    InitPieVectTable();

    // ADC ISR Fonksiyonu
    EALLOW;
    PieVectTable.ADCA1_INT = &adcA1ISR;  // ADCA kesintisi için fonksiyon
    EDIS;

    // ADC yapýlandýr ve güç ver
    initADC();

    // ADC EPWM1 i yapýlandýr
    initEPWM();

    // ADC EPWM1 i tetiklemeli ADC ayarla
    initADCSOC();

    // EPWM2 için
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    HRPWM2_Config(10);      // ePWM2 target, Period = 10

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;

    // ADC için
    // Global kesintileri ve yüksek öncelikli gerçek zamanlý olaylarý etkinleþtir
    IER |= M_INT1;  // Grup 1 kesintilerini etkinleþtir
    EINT;           // Global kesintiyi etkinleþtir
    ERTM;           // Global gerçek zamanlý kesintiyi etkinleþtir

    // PIE kesintisini etkinleþtir
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;

    // ePWM sayacýný baþlat ve ADC tetiklemelerini etkinleþtir
    EALLOW;
    EPwm1Regs.ETSEL.bit.SOCAEN = 1;    // SOCA etkinleþtir
    EPwm1Regs.TBCTL.bit.CTRMODE = 0;   // Sayýcýyý baþlat (up-count mode)
    EDIS;

    while(1)
    {

    }
}


// EPWM içi HRPWM2 fonksiyonu
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

//ADC için fonksiyonlar
void initADC(void)
{
    SetVREF(ADC_ADCA, ADC_INTERNAL, ADC_VREF3P3); // VREF'i dahili olarak ayarla

    EALLOW;
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6;  // ADCCLK bölücüyü 1/4 olarak ayarla
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;  // Geç darbe konumlarýný ayarla
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;  // ADC'yi güçlendir
    EDIS;

    DELAY_US(1000);  // Güçlenme süresi için bekle
}

void initEPWM(void)
{
    EALLOW;
    EPwm1Regs.ETSEL.bit.SOCAEN = 0;     // A grubu için SOC'u devre dýþý býrak
    EPwm1Regs.ETSEL.bit.SOCASEL = 4;    // Yukarý saymada SOC seç
    EPwm1Regs.ETPS.bit.SOCAPRD = 1;     // Ýlk olayda darbe oluþtur
    EPwm1Regs.CMPA.bit.CMPA = 0x0800;   // Karþýlaþtýrma A deðerini ayarla (2048)
    EPwm1Regs.TBPRD = 0x1000;           // Periyodu 4096 olarak ayarla
    EPwm1Regs.TBCTL.bit.CTRMODE = 3;    // Sayýcýyý dondur
    EDIS;
}

void initADCSOC(void)
{
    EALLOW;
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 1;  // SOC0 A1 pinini dönüþtürecek
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = 9;  // Örnekleme penceresi 10 SYSCLK döngüsü
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 5;  // ePWM1 SOCA tarafýndan tetikle
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0;  // SOC0 sonu INT1 bayraðýný ayarla
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;  // INT1 bayraðýný etkinleþtir
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;  // INT1 bayraðýnýn temizlendiðinden emin ol
    EDIS;
}

__interrupt void adcA1ISR(void)
{
    Voltage1 = AdcaResultRegs.ADCRESULT0;  // ADC sonucunu oku

    Dutycycle1 = map(Voltage1,4095,7,0,255 );

    EPwm2Regs.CMPA.bit.CMPAHR = Dutycycle1 << 8;

    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;  // Kesinti bayraðýný temizle

    if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
    {
        AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1; // INT1 taþma bayraðýný temizle
        AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; // INT1 bayraðýný temizle
    }

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;  // PIE kesintisini onayla
}

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


