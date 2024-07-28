//
// Dahil Edilen Dosyalar
//
#include "f28x_project.h"

//
// Tanýmlar
//
#define DEVICE_GPIO_PIN_INPUT1    6
#define DEVICE_GPIO_PIN_INPUT2    10
#define DEVICE_GPIO_PIN_OUTPUT    39

//
// Fonksiyon Prototipleri
//
void btn_blink_setup();
void btn_blink_loop();

//
// Deðiþkenler
//
int buttonState3; // Buton 3 durumunu saklamak için deðiþken
int buttonState4; // Buton 4 durumunu saklamak için deðiþken

int led1_on = 0; // LED'in yanma durumunu saklamak için deðiþken
int timer_started = 0; // Timer'ýn baþladýðýný belirten deðiþken

//
// Kesme Fonksiyonu Prototipleri
//
__interrupt void cpuTimer0ISR(void);

//
// Ana Fonksiyon
//
void main(void)
{
    //
    // Cihaz saatini ve çevre birimlerini baþlat
    //
    InitSysCtrl();

    //
    // GPIO PIN Ayarlarý
    //
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
    PieVectTable.TIMER0_INT = &cpuTimer0ISR;
    EDIS;

    //
    // Cihaz Çevre Birimini Baþlat. Bu örnekte, sadece CPU Zamanlayýcýlarýný baþlat.
    //
    InitCpuTimers();

    //
    // CPU-Zamanlayýcý 0, 1 ve 2'yi her saniye kesme yapacak þekilde yapýlandýr:
    // 80MHz CPU Freq, 1 saniye Periyot (uSaniye cinsinden)
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
    // Global Kesme (INTM) ve gerçek zamanlý kesme (DBGM) etkinleþtir
    //
    EINT;
    ERTM;

    //
    // Sonsuza kadar döngü
    //
    while(1)
    {
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
    GPIO_SetupPinOptions(DEVICE_GPIO_PIN_INPUT1, GPIO_INPUT, 0); // 0 PULLDOWN'a yarýyor

    GPIO_SetupPinMux(DEVICE_GPIO_PIN_INPUT2, GPIO_MUX_CPU1, 0);
    GPIO_SetupPinOptions(DEVICE_GPIO_PIN_INPUT2, GPIO_INPUT, 0); // 0 PULLDOWN'a yarýyor

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
        // Buton basýldý
        GPIO_WritePin(DEVICE_GPIO_PIN_OUTPUT, 1); // GPIO39 çýkýþýný 1 yap (LED'i yak)
        led1_on = 1;
        timer_started = 1;
        CpuTimer0Regs.TCR.bit.TSS = 0; // Timer'ý baþlat
    }
}

//
// cpuTimer0ISR - CPU Timer0 ISR kesme sayacý ile
//
__interrupt void cpuTimer0ISR(void)
{
    static int led1_timer = 0;

    if(timer_started)
    {
        led1_timer++;
        if(led1_timer >= 1) // 1000 ms geçti
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
// Dosyanýn Sonu
//
