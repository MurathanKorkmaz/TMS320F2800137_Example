
#include "f28x_project.h"

uint16_t analogDeger = 0;

void initADC(void);
void initEPWM(void);
void initADCSOC(void);
__interrupt void adcA1ISR(void);

void main(void)
{
    // Cihaz saat ve çevre birimlerini baþlat
    InitSysCtrl();

    // GPIO baþlat
    InitGpio();

    // CPU kesintilerini devre dýþý býrak
    DINT;

    // PIE kontrol kayýtlarýný varsayýlan durumuna ayarla
    InitPieCtrl();

    // CPU kesintilerini devre dýþý býrak ve tüm CPU kesinti bayraklarýný temizle
    IER = 0x0000;
    IFR = 0x0000;

    // PIE vektör tablosunu baþlat
    InitPieVectTable();

    // ISR fonksiyonlarýný eþleþtir
    EALLOW;
    PieVectTable.ADCA1_INT = &adcA1ISR;  // ADCA kesintisi için fonksiyon
    EDIS;

    // ADC yapýlandýr ve güç ver
    initADC();

    // ePWM yapýlandýr
    initEPWM();

    // ePWM tetiklemeli ADC ayarla
    initADCSOC();

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
    analogDeger = AdcaResultRegs.ADCRESULT0;  // ADC sonucunu oku

    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;  // Kesinti bayraðýný temizle

    if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
    {
        AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1; // INT1 taþma bayraðýný temizle
        AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; // INT1 bayraðýný temizle
    }

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;  // PIE kesintisini onayla
}
