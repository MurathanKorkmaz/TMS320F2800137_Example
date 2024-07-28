
#include "f28x_project.h"

uint16_t analogDeger = 0;

void initADC(void);
void initEPWM(void);
void initADCSOC(void);
__interrupt void adcA1ISR(void);

void main(void)
{
    // Cihaz saat ve �evre birimlerini ba�lat
    InitSysCtrl();

    // GPIO ba�lat
    InitGpio();

    // CPU kesintilerini devre d��� b�rak
    DINT;

    // PIE kontrol kay�tlar�n� varsay�lan durumuna ayarla
    InitPieCtrl();

    // CPU kesintilerini devre d��� b�rak ve t�m CPU kesinti bayraklar�n� temizle
    IER = 0x0000;
    IFR = 0x0000;

    // PIE vekt�r tablosunu ba�lat
    InitPieVectTable();

    // ISR fonksiyonlar�n� e�le�tir
    EALLOW;
    PieVectTable.ADCA1_INT = &adcA1ISR;  // ADCA kesintisi i�in fonksiyon
    EDIS;

    // ADC yap�land�r ve g�� ver
    initADC();

    // ePWM yap�land�r
    initEPWM();

    // ePWM tetiklemeli ADC ayarla
    initADCSOC();

    // Global kesintileri ve y�ksek �ncelikli ger�ek zamanl� olaylar� etkinle�tir
    IER |= M_INT1;  // Grup 1 kesintilerini etkinle�tir
    EINT;           // Global kesintiyi etkinle�tir
    ERTM;           // Global ger�ek zamanl� kesintiyi etkinle�tir

    // PIE kesintisini etkinle�tir
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;

    // ePWM sayac�n� ba�lat ve ADC tetiklemelerini etkinle�tir
    EALLOW;
    EPwm1Regs.ETSEL.bit.SOCAEN = 1;    // SOCA etkinle�tir
    EPwm1Regs.TBCTL.bit.CTRMODE = 0;   // Say�c�y� ba�lat (up-count mode)
    EDIS;

    while(1)
    {

    }
}


void initADC(void)
{
    SetVREF(ADC_ADCA, ADC_INTERNAL, ADC_VREF3P3); // VREF'i dahili olarak ayarla

    EALLOW;
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6;  // ADCCLK b�l�c�y� 1/4 olarak ayarla
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1;  // Ge� darbe konumlar�n� ayarla
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;  // ADC'yi g��lendir
    EDIS;

    DELAY_US(1000);  // G��lenme s�resi i�in bekle
}

void initEPWM(void)
{
    EALLOW;
    EPwm1Regs.ETSEL.bit.SOCAEN = 0;     // A grubu i�in SOC'u devre d��� b�rak
    EPwm1Regs.ETSEL.bit.SOCASEL = 4;    // Yukar� saymada SOC se�
    EPwm1Regs.ETPS.bit.SOCAPRD = 1;     // �lk olayda darbe olu�tur
    EPwm1Regs.CMPA.bit.CMPA = 0x0800;   // Kar��la�t�rma A de�erini ayarla (2048)
    EPwm1Regs.TBPRD = 0x1000;           // Periyodu 4096 olarak ayarla
    EPwm1Regs.TBCTL.bit.CTRMODE = 3;    // Say�c�y� dondur
    EDIS;
}

void initADCSOC(void)
{
    EALLOW;
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = 1;  // SOC0 A1 pinini d�n��t�recek
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = 9;  // �rnekleme penceresi 10 SYSCLK d�ng�s�
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 5;  // ePWM1 SOCA taraf�ndan tetikle
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0;  // SOC0 sonu INT1 bayra��n� ayarla
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;  // INT1 bayra��n� etkinle�tir
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;  // INT1 bayra��n�n temizlendi�inden emin ol
    EDIS;
}

__interrupt void adcA1ISR(void)
{
    analogDeger = AdcaResultRegs.ADCRESULT0;  // ADC sonucunu oku

    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;  // Kesinti bayra��n� temizle

    if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
    {
        AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1; // INT1 ta�ma bayra��n� temizle
        AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; // INT1 bayra��n� temizle
    }

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;  // PIE kesintisini onayla
}
