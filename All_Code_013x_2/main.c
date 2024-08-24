
#include "f28x_project.h"

//
// Tan�mlar
//
#define DEVICE_GPIO_PIN_INPUT1    6
#define DEVICE_GPIO_PIN_INPUT2    10
#define DEVICE_GPIO_PIN_OUTPUT    39

void btn_blink_setup();
void btn_blink_loop();

void HRPWM1B_Config(Uint16);
void InitEPwm1BGpio(void);

void HRPWM2B_Config(Uint16);
void InitEPwm2BGpio(void);

//
// Kesme Fonksiyonu Prototipleri
//
__interrupt void cpuTimer0ISR(void);

// ADC fonksiyonlar�n�n tan�mlar�
void initADC(void);
void initEPWM(void);
void initADCSOC(void);
__interrupt void adcA1ISR(void);
long map(long x, long in_min, long in_max, long out_min, long out_max);

//
// De�i�kenler
//
int buttonState3;  // Buton 3 durumunu saklamak i�in de�i�ken
int buttonState4;  // Buton 4 durumunu saklamak i�in de�i�ken

int led1_on = 0;         // LED'in yanma durumunu saklamak i�in de�i�ken
int timer_started = 0;   // Timer'�n ba�lad���n� belirten de�i�ken

// Global de�i�kenler ADC
uint16_t Dutycycle1 = 0, Dutycycle2 = 0, Voltage1 = 0, Voltage2 = 0;

//
// main fonksiyonu
//
void main(void)
{
    //
    // Cihaz saatini ve �evre birimlerini ba�lat
    //
    InitSysCtrl();

    //
    // GPIO ve PWM Pin Ayarlar�
    //
    InitGpio();
    InitEPwm1BGpio();
    InitEPwm2BGpio();
    btn_blink_setup();

    //
    // PIE'yi ba�lat ve PIE kay�tlar�n� temizle. CPU kesmelerini devre d��� b�rak.
    //
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;

    //
    // PIE vekt�r tablosunu shell Kesme Servis Rutinlerine (ISR) i�aret�ilerle ba�lat.
    //
    InitPieVectTable();

    //
    // ISR fonksiyonlar�n� e�le
    //
    EALLOW;
    PieVectTable.TIMER0_INT = &cpuTimer0ISR;  // Timer0 kesme servisi rutini
    PieVectTable.ADCA1_INT = &adcA1ISR;       // ADCA kesme servisi rutini
    EDIS;

    //
    // Cihaz �evre Birimini Ba�lat. Bu �rnekte, sadece CPU Zamanlay�c�lar�n� ba�lat.
    //
    InitCpuTimers();

    //
    // CPU-Zamanlay�c� 0, 1 ve 2'yi her saniye kesme yapacak �ekilde yap�land�r:
    // 80MHz CPU Freq, 1 saniye Periyot (mikrosaniye cinsinden)
    //
    ConfigCpuTimer(&CpuTimer0, 80, 1000000);

    //
    // Hassas zamanlama sa�lamak i�in, t�m kay�t defterine yazmak i�in sadece yazma
    // talimatlar�n� kullan. Bu nedenle, ConfigCpuTimer ve InitCpuTimers'ta
    // yap�land�rma bitlerinden herhangi biri de�i�irse, a�a��daki ayarlar�n da
    // g�ncellenmesi gerekir.
    //
    CpuTimer0Regs.TCR.all = 0x4000;

    //
    // CPU-Zamanlay�c� 0'a ba�l� olan CPU int1'i, CPU-Zamanlay�c� 1'e ba�l� olan
    // CPU int13'� ve CPU-Zamanlay�c� 2'ye ba�l� olan CPU int14'� etkinle�tir.
    //
    IER |= M_INT1;

    //
    // PIE'de TINT0'� etkinle�tir: Grup 1 kesmesi 7
    //
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;

    //
    // ADC yap�land�r ve g�� ver
    //
    initADC();

    //
    // ADC EPWM1'i yap�land�r
    //
    initEPWM();

    //
    // ADC EPWM1'i tetiklemeli ADC ayarla
    //
    initADCSOC();

    //
    // PWM Mod�lleri Yap�land�r
    //
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;  // TBCLK senkronizasyonunu devre d��� b�rak
    EDIS;

    HRPWM1B_Config(10);  // ePWM1b hedefi, Periyot = 10
    HRPWM2B_Config(10);  // ePWM2b hedefi, Periyot = 10

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;  // TBCLK senkronizasyonunu etkinle�tir
    EDIS;

    //
    // ADC i�in
    // Global kesintileri ve y�ksek �ncelikli ger�ek zamanl� olaylar� etkinle�tir
    //
    IER |= M_INT1;  // Grup 1 kesintilerini etkinle�tir
    EINT;           // Global kesintiyi etkinle�tir
    ERTM;           // Global ger�ek zamanl� kesintiyi etkinle�tir

    //
    // PIE kesintisini etkinle�tir
    //
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;

    //
    // ePWM sayac�n� ba�lat ve ADC tetiklemelerini etkinle�tir
    //
    EALLOW;
    EPwm3Regs.ETSEL.bit.SOCAEN = 1;    // SOCA'y� etkinle�tir
    EPwm3Regs.TBCTL.bit.CTRMODE = 0;   // Say�c�y� ba�lat (yukar� say�m modu)
    EDIS;

    while(1)
    {
        // Ana d�ng�: ��lemler burada ger�ekle�tirilecektir.
        // GPIO pin durumlar�n� kontrol edin ve gerekti�inde PWM ��k��lar�n� y�netin.
        btn_blink_loop();
    }
}

//
// btn_blink_setup Fonksiyon A��klamas�
//
void btn_blink_setup()
{
    //
    // GPIO'yu ba�lat ve GPIO pinlerini yap�land�r
    //
    InitGpio();

    // GPIO6 ve GPIO10'u pull-down diren�li dijital giri� olarak ayarla
    GPIO_SetupPinMux(DEVICE_GPIO_PIN_INPUT1, GPIO_MUX_CPU1, 0);
    GPIO_SetupPinOptions(DEVICE_GPIO_PIN_INPUT1, GPIO_INPUT, GPIO_PULLUP);

    GPIO_SetupPinMux(DEVICE_GPIO_PIN_INPUT2, GPIO_MUX_CPU1, 0);
    GPIO_SetupPinOptions(DEVICE_GPIO_PIN_INPUT2, GPIO_INPUT, GPIO_PULLUP);

    // GPIO39'u dijital ��k�� olarak ayarla
    GPIO_SetupPinMux(DEVICE_GPIO_PIN_OUTPUT, GPIO_MUX_CPU1, 0);
    GPIO_SetupPinOptions(DEVICE_GPIO_PIN_OUTPUT, GPIO_OUTPUT, GPIO_PUSHPULL);
}

//
// btn_blink_loop Fonksiyon A��klamas�
//
void btn_blink_loop()
{
    buttonState3 = GPIO_ReadPin(DEVICE_GPIO_PIN_INPUT1); // GPIO6 pinini oku
    buttonState4 = GPIO_ReadPin(DEVICE_GPIO_PIN_INPUT2); // GPIO10 pinini oku

    if((buttonState3 == 1 || buttonState4 == 1) && !led1_on)
    {
        // Butonlardan birine bas�ld�ysa ve LED yanm�yorsa
        GPIO_WritePin(DEVICE_GPIO_PIN_OUTPUT, 1); // GPIO39 ��k���n� 1 yap (LED'i yak)
        led1_on = 1;
        timer_started = 1;
        CpuTimer0Regs.TCR.bit.TSS = 0; // Timer'� ba�lat
    }
}

//
// HRPWM1B_Config - HRPWM1B yap�land�rma fonksiyonu
//
void HRPWM1B_Config(Uint16 period)
{
    //
    // ePWM1 register konfig�rasyonu HRPWM ile
    // ePWM1B d���k/y�ksek seviyeye ge�i� MEP kontrol� ile Y�kselen kenarda
    //
    EPwm1Regs.TBCTL.bit.PRDLD = TB_IMMEDIATE;    // An�nda y�kleme ayarla
    EPwm1Regs.TBPRD = period - 1;                // PWM frekans� = 1 / periyot
    EPwm1Regs.CMPB.bit.CMPB = period / 2;        // Ba�lang��ta %50 g�rev ayarla
    EPwm1Regs.TBPHS.all = 0;                     // Faz� s�f�rla
    EPwm1Regs.TBCTR = 0;                         // Sayac� s�f�rla

    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;   // Yukar� say�m modu
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;      // EPwm1 Master olarak ayarla
    EPwm1Regs.EPWMSYNCINSEL.all = SYNC_IN_SRC_DISABLE_ALL; // T�m senkron giri� kaynaklar�n� devre d��� b�rak
    EPwm1Regs.EPWMSYNCOUTEN.all = SYNC_OUT_SRC_DISABLE_ALL; // T�m senkron ��k�� kaynaklar�n� devre d��� b�rak
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;     // Y�ksek h�zl� �n b�l�c� ayarla
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;        // Saat �n b�l�c�s�n� ayarla

    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO; // CMPB g�ncellemesi s�f�rda
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;   // G�lge modu etkin

    EPwm1Regs.AQCTLB.bit.ZRO = AQ_CLEAR;        // PWM d���k/y�ksek de�i�tir
    EPwm1Regs.AQCTLB.bit.CBU = AQ_SET;          // Y�kselen kenarda set

    EALLOW;
    EPwm1Regs.HRCNFG.all = 0x0;
    EPwm1Regs.HRCNFG.bit.EDGMODE = HR_REP;      // Y�kselen kenarda MEP kontrol�
    EPwm1Regs.HRCNFG.bit.CTLMODE = HR_CMP;      // HRPWM CMPB ile kontrol
    EPwm1Regs.HRCNFG.bit.HRLOAD  = HR_CTR_ZERO; // HR y�kleme s�f�rda
    EDIS;
}

//
// HRPWM2B_Config - HRPWM2B yap�land�rma fonksiyonu
//
void HRPWM2B_Config(Uint16 period)
{
    //
    // ePWM2 register konfig�rasyonu HRPWM ile
    // ePWM2B d���k/y�ksek seviyeye ge�i� MEP kontrol� ile Y�kselen kenarda
    //
    EPwm2Regs.TBCTL.bit.PRDLD = TB_IMMEDIATE;    // An�nda y�kleme ayarla
    EPwm2Regs.TBPRD = period - 1;                // PWM frekans� = 1 / periyot
    EPwm2Regs.CMPB.bit.CMPB = period / 2;        // Ba�lang��ta %50 g�rev ayarla
    EPwm2Regs.TBPHS.all = 0;                     // Faz� s�f�rla
    EPwm2Regs.TBCTR = 0;                         // Sayac� s�f�rla

    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;   // Yukar� say�m modu
    EPwm2Regs.TBCTL.bit.PHSEN = TB_DISABLE;      // EPwm2 Master olarak ayarla
    EPwm2Regs.EPWMSYNCINSEL.all = SYNC_IN_SRC_DISABLE_ALL; // T�m senkron giri� kaynaklar�n� devre d��� b�rak
    EPwm2Regs.EPWMSYNCOUTEN.all = SYNC_OUT_SRC_DISABLE_ALL; // T�m senkron ��k�� kaynaklar�n� devre d��� b�rak
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;     // Y�ksek h�zl� �n b�l�c� ayarla
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;        // Saat �n b�l�c�s�n� ayarla

    EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO; // CMPB g�ncellemesi s�f�rda
    EPwm2Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;   // G�lge modu etkin

    EPwm2Regs.AQCTLB.bit.ZRO = AQ_CLEAR;        // PWM d���k/y�ksek de�i�tir
    EPwm2Regs.AQCTLB.bit.CBU = AQ_SET;          // Y�kselen kenarda set

    EALLOW;
    EPwm2Regs.HRCNFG.all = 0x0;
    EPwm2Regs.HRCNFG.bit.EDGMODE = HR_REP;      // Y�kselen kenarda MEP kontrol�
    EPwm2Regs.HRCNFG.bit.CTLMODE = HR_CMP;      // HRPWM CMPB ile kontrol
    EPwm2Regs.HRCNFG.bit.HRLOAD  = HR_CTR_ZERO; // HR y�kleme s�f�rda
    EDIS;
}

//
// InitEPwm1BGpio - EPWM1B i�in GPIO yap�land�rmas�
//
void InitEPwm1BGpio(void)
{
    EALLOW;

    GpioCtrlRegs.GPHPUD.bit.GPIO226 = 1;  // GPIO226'da pull-up direncini devre d��� b�rak

    GpioCtrlRegs.GPHMUX1.bit.GPIO226 = 9;  // GPIO226'y� EPWM1B olarak yap�land�r

    EDIS;
}

//
// InitEPwm2BGpio - EPWM2B i�in GPIO yap�land�rmas�
//
void InitEPwm2BGpio(void)
{
    EALLOW;

    GpioCtrlRegs.GPHPUD.bit.GPIO228 = 1;  // GPIO228'de pull-up direncini devre d��� b�rak

    GpioCtrlRegs.GPHMUX1.bit.GPIO228 = 9;  // GPIO228'i EPWM2B olarak yap�land�r

    EDIS;
}

// ADC yap�land�rma fonksiyonlar�
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

//
// initEPWM - EPWM mod�l�n� yap�land�r
//
void initEPWM(void)
{
    EALLOW;
    EPwm3Regs.ETSEL.bit.SOCAEN = 0;     // A grubu i�in SOC'u devre d��� b�rak
    EPwm3Regs.ETSEL.bit.SOCASEL = 4;    // Yukar� saymada SOC se�
    EPwm3Regs.ETPS.bit.SOCAPRD = 1;     // �lk olayda darbe olu�tur
    EPwm3Regs.CMPA.bit.CMPA = 0x0800;   // Kar��la�t�rma A de�erini ayarla (2048)
    EPwm3Regs.TBPRD = 0x1000;           // Periyodu 4096 olarak ayarla
    EPwm3Regs.TBCTL.bit.CTRMODE = 3;    // Say�c�y� dondur
    EDIS;
}

//
// initADCSOC - ADC SOC yap�land�rma fonksiyonu
//
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

//
// cpuTimer0ISR - CPU Timer0 ISR kesme sayac� ile
//
__interrupt void cpuTimer0ISR(void)
{
    static int led1_timer = 0;  // LED zamanlay�c� de�i�keni

    if(timer_started)
    {
        led1_timer++;
        if(led1_timer >= 1000) // 1000 ms ge�ti
        {
            GPIO_WritePin(DEVICE_GPIO_PIN_OUTPUT, 0); // GPIO39 ��k���n� 0 yap (LED'i s�nd�r)
            led1_timer = 0;
            led1_on = 0;
            timer_started = 0;
            CpuTimer0Regs.TCR.bit.TSS = 1; // Timer'� durdur
        }
    }

    CpuTimer0.InterruptCount++;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; // PIE kesme onay�n� ver
}

//
// adcA1ISR - ADC A1 ISR fonksiyonu
//
__interrupt void adcA1ISR(void)
{
    Voltage1 = AdcaResultRegs.ADCRESULT0;  // ADC sonucunu oku
    Voltage2 = AdcaResultRegs.ADCRESULT0;  // ADC sonucunu oku

    // ADC voltaj�n� g�rev d�ng�s�ne e�le
    Dutycycle1 = map(Voltage1, 0, 4095, 0, 255);
    Dutycycle2 = map(Voltage2, 0, 4095, 0, 255);

    // PWM g�rev d�ng�s�n� ayarla
    EPwm1Regs.CMPB.bit.CMPBHR = Dutycycle1 << 8;
    EPwm2Regs.CMPB.bit.CMPBHR = Dutycycle2 << 8;

    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;  // Kesinti bayra��n� temizle

    if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
    {
        AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1; // INT1 ta�ma bayra��n� temizle
        AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; // INT1 bayra��n� temizle
    }

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;  // PIE kesintisini onayla
}

//
// map - De�erleri belirli bir aral�kta e�le�tirir
//
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

