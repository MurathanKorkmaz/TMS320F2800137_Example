//
// Dahil Edilen Dosyalar
//
#include "f28x_project.h"

//
// Tan�mlar
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
// De�i�kenler
//
int buttonState3; // Buton 3 durumunu saklamak i�in de�i�ken
int buttonState4; // Buton 4 durumunu saklamak i�in de�i�ken

int led1_on = 0; // LED'in yanma durumunu saklamak i�in de�i�ken
int timer_started = 0; // Timer'�n ba�lad���n� belirten de�i�ken

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
    // Cihaz saatini ve �evre birimlerini ba�lat
    //
    InitSysCtrl();

    //
    // GPIO PIN Ayarlar�
    //
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
    PieVectTable.TIMER0_INT = &cpuTimer0ISR;
    EDIS;

    //
    // Cihaz �evre Birimini Ba�lat. Bu �rnekte, sadece CPU Zamanlay�c�lar�n� ba�lat.
    //
    InitCpuTimers();

    //
    // CPU-Zamanlay�c� 0, 1 ve 2'yi her saniye kesme yapacak �ekilde yap�land�r:
    // 80MHz CPU Freq, 1 saniye Periyot (uSaniye cinsinden)
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
    // Global Kesme (INTM) ve ger�ek zamanl� kesme (DBGM) etkinle�tir
    //
    EINT;
    ERTM;

    //
    // Sonsuza kadar d�ng�
    //
    while(1)
    {
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
    GPIO_SetupPinOptions(DEVICE_GPIO_PIN_INPUT1, GPIO_INPUT, 0); // 0 PULLDOWN'a yar�yor

    GPIO_SetupPinMux(DEVICE_GPIO_PIN_INPUT2, GPIO_MUX_CPU1, 0);
    GPIO_SetupPinOptions(DEVICE_GPIO_PIN_INPUT2, GPIO_INPUT, 0); // 0 PULLDOWN'a yar�yor

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
        // Buton bas�ld�
        GPIO_WritePin(DEVICE_GPIO_PIN_OUTPUT, 1); // GPIO39 ��k���n� 1 yap (LED'i yak)
        led1_on = 1;
        timer_started = 1;
        CpuTimer0Regs.TCR.bit.TSS = 0; // Timer'� ba�lat
    }
}

//
// cpuTimer0ISR - CPU Timer0 ISR kesme sayac� ile
//
__interrupt void cpuTimer0ISR(void)
{
    static int led1_timer = 0;

    if(timer_started)
    {
        led1_timer++;
        if(led1_timer >= 1) // 1000 ms ge�ti
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
// Dosyan�n Sonu
//
