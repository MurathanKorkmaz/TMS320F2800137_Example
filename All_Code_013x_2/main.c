
#include "f28x_project.h"

//
// Tanýmlar
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

// ADC fonksiyonlarýnýn tanýmlarý
void initADC(void);
void initEPWM(void);
void initADCSOC(void);
__interrupt void adcA1ISR(void);
long map(long x, long in_min, long in_max, long out_min, long out_max);

//
// Deðiþkenler
//
int buttonState3;  // Buton 3 durumunu saklamak için deðiþken
int buttonState4;  // Buton 4 durumunu saklamak için deðiþken

int led1_on = 0;         // LED'in yanma durumunu saklamak için deðiþken
int timer_started = 0;   // Timer'ýn baþladýðýný belirten deðiþken

// Global deðiþkenler ADC
uint16_t Dutycycle1 = 0, Dutycycle2 = 0, Voltage1 = 0, Voltage2 = 0;

//
// main fonksiyonu
//
void main(void)
{
    //
    // Cihaz saatini ve çevre birimlerini baþlat
    //
    InitSysCtrl();

    //
    // GPIO ve PWM Pin Ayarlarý
    //
    InitGpio();
    InitEPwm1BGpio();
    InitEPwm2BGpio();
    btn_blink_setup();

    //
    // PIE'yi baþlat ve PIE kayýtlarýný temizle. CPU kesmelerini devre dýþý býrak.
    //
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;

    //
    // PIE vektör tablosunu shell Kesme Servis Rutinlerine (ISR) iþaretçilerle baþlat.
    //
    InitPieVectTable();

    //
    // ISR fonksiyonlarýný eþle
    //
    EALLOW;
    PieVectTable.TIMER0_INT = &cpuTimer0ISR;  // Timer0 kesme servisi rutini
    PieVectTable.ADCA1_INT = &adcA1ISR;       // ADCA kesme servisi rutini
    EDIS;

    //
    // Cihaz Çevre Birimini Baþlat. Bu örnekte, sadece CPU Zamanlayýcýlarýný baþlat.
    //
    InitCpuTimers();

    //
    // CPU-Zamanlayýcý 0, 1 ve 2'yi her saniye kesme yapacak þekilde yapýlandýr:
    // 80MHz CPU Freq, 1 saniye Periyot (mikrosaniye cinsinden)
    //
    ConfigCpuTimer(&CpuTimer0, 80, 1000000);

    //
    // Hassas zamanlama saðlamak için, tüm kayýt defterine yazmak için sadece yazma
    // talimatlarýný kullan. Bu nedenle, ConfigCpuTimer ve InitCpuTimers'ta
    // yapýlandýrma bitlerinden herhangi biri deðiþirse, aþaðýdaki ayarlarýn da
    // güncellenmesi gerekir.
    //
    CpuTimer0Regs.TCR.all = 0x4000;

    //
    // CPU-Zamanlayýcý 0'a baðlý olan CPU int1'i, CPU-Zamanlayýcý 1'e baðlý olan
    // CPU int13'ü ve CPU-Zamanlayýcý 2'ye baðlý olan CPU int14'ü etkinleþtir.
    //
    IER |= M_INT1;

    //
    // PIE'de TINT0'ý etkinleþtir: Grup 1 kesmesi 7
    //
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;

    //
    // ADC yapýlandýr ve güç ver
    //
    initADC();

    //
    // ADC EPWM1'i yapýlandýr
    //
    initEPWM();

    //
    // ADC EPWM1'i tetiklemeli ADC ayarla
    //
    initADCSOC();

    //
    // PWM Modülleri Yapýlandýr
    //
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;  // TBCLK senkronizasyonunu devre dýþý býrak
    EDIS;

    HRPWM1B_Config(10);  // ePWM1b hedefi, Periyot = 10
    HRPWM2B_Config(10);  // ePWM2b hedefi, Periyot = 10

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;  // TBCLK senkronizasyonunu etkinleþtir
    EDIS;

    //
    // ADC için
    // Global kesintileri ve yüksek öncelikli gerçek zamanlý olaylarý etkinleþtir
    //
    IER |= M_INT1;  // Grup 1 kesintilerini etkinleþtir
    EINT;           // Global kesintiyi etkinleþtir
    ERTM;           // Global gerçek zamanlý kesintiyi etkinleþtir

    //
    // PIE kesintisini etkinleþtir
    //
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;

    //
    // ePWM sayacýný baþlat ve ADC tetiklemelerini etkinleþtir
    //
    EALLOW;
    EPwm3Regs.ETSEL.bit.SOCAEN = 1;    // SOCA'yý etkinleþtir
    EPwm3Regs.TBCTL.bit.CTRMODE = 0;   // Sayýcýyý baþlat (yukarý sayým modu)
    EDIS;

    while(1)
    {
        // Ana döngü: Ýþlemler burada gerçekleþtirilecektir.
        // GPIO pin durumlarýný kontrol edin ve gerektiðinde PWM çýkýþlarýný yönetin.
        btn_blink_loop();
    }
}

//
// btn_blink_setup Fonksiyon Açýklamasý
//
void btn_blink_setup()
{
    //
    // GPIO'yu baþlat ve GPIO pinlerini yapýlandýr
    //
    InitGpio();

    // GPIO6 ve GPIO10'u pull-down dirençli dijital giriþ olarak ayarla
    GPIO_SetupPinMux(DEVICE_GPIO_PIN_INPUT1, GPIO_MUX_CPU1, 0);
    GPIO_SetupPinOptions(DEVICE_GPIO_PIN_INPUT1, GPIO_INPUT, GPIO_PULLUP);

    GPIO_SetupPinMux(DEVICE_GPIO_PIN_INPUT2, GPIO_MUX_CPU1, 0);
    GPIO_SetupPinOptions(DEVICE_GPIO_PIN_INPUT2, GPIO_INPUT, GPIO_PULLUP);

    // GPIO39'u dijital çýkýþ olarak ayarla
    GPIO_SetupPinMux(DEVICE_GPIO_PIN_OUTPUT, GPIO_MUX_CPU1, 0);
    GPIO_SetupPinOptions(DEVICE_GPIO_PIN_OUTPUT, GPIO_OUTPUT, GPIO_PUSHPULL);
}

//
// btn_blink_loop Fonksiyon Açýklamasý
//
void btn_blink_loop()
{
    buttonState3 = GPIO_ReadPin(DEVICE_GPIO_PIN_INPUT1); // GPIO6 pinini oku
    buttonState4 = GPIO_ReadPin(DEVICE_GPIO_PIN_INPUT2); // GPIO10 pinini oku

    if((buttonState3 == 1 || buttonState4 == 1) && !led1_on)
    {
        // Butonlardan birine basýldýysa ve LED yanmýyorsa
        GPIO_WritePin(DEVICE_GPIO_PIN_OUTPUT, 1); // GPIO39 çýkýþýný 1 yap (LED'i yak)
        led1_on = 1;
        timer_started = 1;
        CpuTimer0Regs.TCR.bit.TSS = 0; // Timer'ý baþlat
    }
}

//
// HRPWM1B_Config - HRPWM1B yapýlandýrma fonksiyonu
//
void HRPWM1B_Config(Uint16 period)
{
    //
    // ePWM1 register konfigürasyonu HRPWM ile
    // ePWM1B düþük/yüksek seviyeye geçiþ MEP kontrolü ile Yükselen kenarda
    //
    EPwm1Regs.TBCTL.bit.PRDLD = TB_IMMEDIATE;    // Anýnda yükleme ayarla
    EPwm1Regs.TBPRD = period - 1;                // PWM frekansý = 1 / periyot
    EPwm1Regs.CMPB.bit.CMPB = period / 2;        // Baþlangýçta %50 görev ayarla
    EPwm1Regs.TBPHS.all = 0;                     // Fazý sýfýrla
    EPwm1Regs.TBCTR = 0;                         // Sayacý sýfýrla

    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;   // Yukarý sayým modu
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;      // EPwm1 Master olarak ayarla
    EPwm1Regs.EPWMSYNCINSEL.all = SYNC_IN_SRC_DISABLE_ALL; // Tüm senkron giriþ kaynaklarýný devre dýþý býrak
    EPwm1Regs.EPWMSYNCOUTEN.all = SYNC_OUT_SRC_DISABLE_ALL; // Tüm senkron çýkýþ kaynaklarýný devre dýþý býrak
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;     // Yüksek hýzlý ön bölücü ayarla
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;        // Saat ön bölücüsünü ayarla

    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO; // CMPB güncellemesi sýfýrda
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;   // Gölge modu etkin

    EPwm1Regs.AQCTLB.bit.ZRO = AQ_CLEAR;        // PWM düþük/yüksek deðiþtir
    EPwm1Regs.AQCTLB.bit.CBU = AQ_SET;          // Yükselen kenarda set

    EALLOW;
    EPwm1Regs.HRCNFG.all = 0x0;
    EPwm1Regs.HRCNFG.bit.EDGMODE = HR_REP;      // Yükselen kenarda MEP kontrolü
    EPwm1Regs.HRCNFG.bit.CTLMODE = HR_CMP;      // HRPWM CMPB ile kontrol
    EPwm1Regs.HRCNFG.bit.HRLOAD  = HR_CTR_ZERO; // HR yükleme sýfýrda
    EDIS;
}

//
// HRPWM2B_Config - HRPWM2B yapýlandýrma fonksiyonu
//
void HRPWM2B_Config(Uint16 period)
{
    //
    // ePWM2 register konfigürasyonu HRPWM ile
    // ePWM2B düþük/yüksek seviyeye geçiþ MEP kontrolü ile Yükselen kenarda
    //
    EPwm2Regs.TBCTL.bit.PRDLD = TB_IMMEDIATE;    // Anýnda yükleme ayarla
    EPwm2Regs.TBPRD = period - 1;                // PWM frekansý = 1 / periyot
    EPwm2Regs.CMPB.bit.CMPB = period / 2;        // Baþlangýçta %50 görev ayarla
    EPwm2Regs.TBPHS.all = 0;                     // Fazý sýfýrla
    EPwm2Regs.TBCTR = 0;                         // Sayacý sýfýrla

    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UP;   // Yukarý sayým modu
    EPwm2Regs.TBCTL.bit.PHSEN = TB_DISABLE;      // EPwm2 Master olarak ayarla
    EPwm2Regs.EPWMSYNCINSEL.all = SYNC_IN_SRC_DISABLE_ALL; // Tüm senkron giriþ kaynaklarýný devre dýþý býrak
    EPwm2Regs.EPWMSYNCOUTEN.all = SYNC_OUT_SRC_DISABLE_ALL; // Tüm senkron çýkýþ kaynaklarýný devre dýþý býrak
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;     // Yüksek hýzlý ön bölücü ayarla
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;        // Saat ön bölücüsünü ayarla

    EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO; // CMPB güncellemesi sýfýrda
    EPwm2Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;   // Gölge modu etkin

    EPwm2Regs.AQCTLB.bit.ZRO = AQ_CLEAR;        // PWM düþük/yüksek deðiþtir
    EPwm2Regs.AQCTLB.bit.CBU = AQ_SET;          // Yükselen kenarda set

    EALLOW;
    EPwm2Regs.HRCNFG.all = 0x0;
    EPwm2Regs.HRCNFG.bit.EDGMODE = HR_REP;      // Yükselen kenarda MEP kontrolü
    EPwm2Regs.HRCNFG.bit.CTLMODE = HR_CMP;      // HRPWM CMPB ile kontrol
    EPwm2Regs.HRCNFG.bit.HRLOAD  = HR_CTR_ZERO; // HR yükleme sýfýrda
    EDIS;
}

//
// InitEPwm1BGpio - EPWM1B için GPIO yapýlandýrmasý
//
void InitEPwm1BGpio(void)
{
    EALLOW;

    GpioCtrlRegs.GPHPUD.bit.GPIO226 = 1;  // GPIO226'da pull-up direncini devre dýþý býrak

    GpioCtrlRegs.GPHMUX1.bit.GPIO226 = 9;  // GPIO226'yý EPWM1B olarak yapýlandýr

    EDIS;
}

//
// InitEPwm2BGpio - EPWM2B için GPIO yapýlandýrmasý
//
void InitEPwm2BGpio(void)
{
    EALLOW;

    GpioCtrlRegs.GPHPUD.bit.GPIO228 = 1;  // GPIO228'de pull-up direncini devre dýþý býrak

    GpioCtrlRegs.GPHMUX1.bit.GPIO228 = 9;  // GPIO228'i EPWM2B olarak yapýlandýr

    EDIS;
}

// ADC yapýlandýrma fonksiyonlarý
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

//
// initEPWM - EPWM modülünü yapýlandýr
//
void initEPWM(void)
{
    EALLOW;
    EPwm3Regs.ETSEL.bit.SOCAEN = 0;     // A grubu için SOC'u devre dýþý býrak
    EPwm3Regs.ETSEL.bit.SOCASEL = 4;    // Yukarý saymada SOC seç
    EPwm3Regs.ETPS.bit.SOCAPRD = 1;     // Ýlk olayda darbe oluþtur
    EPwm3Regs.CMPA.bit.CMPA = 0x0800;   // Karþýlaþtýrma A deðerini ayarla (2048)
    EPwm3Regs.TBPRD = 0x1000;           // Periyodu 4096 olarak ayarla
    EPwm3Regs.TBCTL.bit.CTRMODE = 3;    // Sayýcýyý dondur
    EDIS;
}

//
// initADCSOC - ADC SOC yapýlandýrma fonksiyonu
//
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

//
// cpuTimer0ISR - CPU Timer0 ISR kesme sayacý ile
//
__interrupt void cpuTimer0ISR(void)
{
    static int led1_timer = 0;  // LED zamanlayýcý deðiþkeni

    if(timer_started)
    {
        led1_timer++;
        if(led1_timer >= 1000) // 1000 ms geçti
        {
            GPIO_WritePin(DEVICE_GPIO_PIN_OUTPUT, 0); // GPIO39 çýkýþýný 0 yap (LED'i söndür)
            led1_timer = 0;
            led1_on = 0;
            timer_started = 0;
            CpuTimer0Regs.TCR.bit.TSS = 1; // Timer'ý durdur
        }
    }

    CpuTimer0.InterruptCount++;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; // PIE kesme onayýný ver
}

//
// adcA1ISR - ADC A1 ISR fonksiyonu
//
__interrupt void adcA1ISR(void)
{
    Voltage1 = AdcaResultRegs.ADCRESULT0;  // ADC sonucunu oku
    Voltage2 = AdcaResultRegs.ADCRESULT0;  // ADC sonucunu oku

    // ADC voltajýný görev döngüsüne eþle
    Dutycycle1 = map(Voltage1, 0, 4095, 0, 255);
    Dutycycle2 = map(Voltage2, 0, 4095, 0, 255);

    // PWM görev döngüsünü ayarla
    EPwm1Regs.CMPB.bit.CMPBHR = Dutycycle1 << 8;
    EPwm2Regs.CMPB.bit.CMPBHR = Dutycycle2 << 8;

    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;  // Kesinti bayraðýný temizle

    if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
    {
        AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1; // INT1 taþma bayraðýný temizle
        AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1; // INT1 bayraðýný temizle
    }

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;  // PIE kesintisini onayla
}

//
// map - Deðerleri belirli bir aralýkta eþleþtirir
//
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

