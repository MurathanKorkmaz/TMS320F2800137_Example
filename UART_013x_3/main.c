//
// Dahil Edilen Dosyalar
//
#include "f28x_project.h"

//
// Global Deðiþkenler
//
uint16_t loopCounter = 0;

//
// Fonksiyon Prototipleri
//
void initSCIAEchoback(void);
void transmitSCIADWord(uint32_t data);
void initSCIAFIFO(void);
void transmitSCIAChar(uint16_t a);

// Fonksiyon Tanýmlamalarý GPIO SCI() GPIO28(RX)-GPIO29(TX)
void InitSciaGpio28_29(void);

//
// CPU Zamanlayýcý0
//

//
// Kesme Fonksiyonu Prototipleri
//
__interrupt void cpuTimer0ISR(void);

//
// millis Açýklamasý
//
volatile unsigned long millisCounter = 0; // Milisaniye sayacýný saklamak için deðiþken

//
// Ana fonksiyon
//
void main(void)
{
    //
    // Cihaz saatini ve çevre birimlerini baþlat
    //
    InitSysCtrl();

    //
    // GPIO28-GPIO29'u baþlat
    //
    InitSciaGpio28_29();

    //
    // CPU kesmelerini devre dýþý býrak
    //
    DINT;

    //
    // PIE kontrol kayýtlarýný varsayýlan durumlarýna baþlat.
    // Varsayýlan durum, tüm PIE kesmelerinin devre dýþý olduðu ve bayraklarýn temizlendiði durumdur.
    //
    InitPieCtrl();

    //
    // CPU kesmelerini devre dýþý býrak ve tüm CPU kesme bayraklarýný temizle
    //
    IER = 0x0000;
    IFR = 0x0000;

    //
    // PIE vektör tablosunu kabuk Kesme Servis Rutinlerine (ISR) iþaretçilerle baþlat
    //
    InitPieVectTable();

    //
    // ISR fonksiyonlarýný eþle
    //
    EALLOW;
    PieVectTable.TIMER0_INT = &cpuTimer0ISR;
    EDIS;

    //
    // Cihaz çevre birimlerini baþlat. Bu örnekte, sadece CPU Zamanlayýcýlarýný baþlat.
    //
    InitCpuTimers();

    //
    // CPU-Zamanlayýcý 0'ý her milisaniyede bir kesme yapacak þekilde yapýlandýr:
    // 80MHz CPU Freq, 1 milisaniye Periyot (mikrosaniye cinsinden)
    //
    ConfigCpuTimer(&CpuTimer0, 80, 1000);

    //
    // Hassas zamanlama saðlamak için, tüm kayýt defterine yazmak için sadece yazma
    // talimatlarýný kullan. Bu nedenle, ConfigCpuTimer ve InitCpuTimers'ta
    // yapýlandýrma bitlerinden herhangi biri deðiþirse, aþaðýdaki ayarlarýn da
    // güncellenmesi gerekir.
    //
    CpuTimer0Regs.TCR.all = 0x4000;

    //
    // CPU-Zamanlayýcý 0'a baðlý olan CPU int1'i etkinleþtir.
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

    loopCounter = 0;

    initSCIAFIFO(); // SCI FIFO'yu baþlat
    initSCIAEchoback(); // SCI'yi ekoyla geri gönderim için baþlat

    while(1)
    {
        // Ana döngü burada boþ býrakýldý, iþlem kesme içinde yapýlacak
        // Ýþlemler her saniye kesme içinde gerçekleþecek
    }
}

//
//  initSCIAEGPIO - GPIO28(RX)-GPIO29(TX)'i Baþlat
//
void InitSciaGpio28_29(void)
{
    EALLOW;

    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;  // GPIO28 için pull-up etkinleþtir (SCIRXDA)
    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;  // GPIO29 için pull-up etkinleþtir (SCITXDA)

    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3;  // GPIO28 için asenkron giriþ seç (SCIRXDA)

    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;  // GPIO28'i SCIRXDA iþlemi için yapýlandýr
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;  // GPIO29'u SCITXDA iþlemi için yapýlandýr

    EDIS;
}


//
//  initSCIAEchoback - SCI-A'yý ekoyla geri gönderim için baþlat
//
void initSCIAEchoback(void)
{
    //
    // Not: SCIA çevre birimine saatler InitSysCtrl() fonksiyonunda açýldý
    //
    SciaRegs.SCICCR.all = 0x0007;           // 1 stop bit, döngü yok
                                            // Parite yok, 8 karakter biti,
                                            // asenkron mod, boþta hat protokolü
    SciaRegs.SCICTL1.all = 0x0003;          // TX, RX, iç SCICLK etkin,
                                            // RX Hata yok, UYKU, TXWAKE devre dýþý
    SciaRegs.SCICTL2.all = 0x0003;
    SciaRegs.SCICTL2.bit.TXINTENA = 1;
    SciaRegs.SCICTL2.bit.RXBKINTENA = 1;

    //
    // 9600 baud'da SCIA
    // @LSPCLK = 25 MHz (100 MHz SYSCLK) HBAUD = 0x01  ve LBAUD = 0x44.
    //
    SciaRegs.SCIHBAUD.all = 0x0001;
    SciaRegs.SCILBAUD.all = 0x0044;

    SciaRegs.SCICTL1.all = 0x0023;          // SCI'yi Reset'ten Çýkar
}

//
// transmitSCIADWord - 32-bit bir deðiþkende saklanan 24-bit deðeri iletir
//
void transmitSCIADWord(uint32_t data)
{
    uint16_t byte1, byte2, byte3;

    // 24-bit veri çýkartýlýr
    byte1 = (data >> 16) & 0xFF; // Üst 8 bit
    byte2 = (data >> 8) & 0xFF;  // Orta 8 bit
    byte3 = data & 0xFF;         // Alt 8 bit

    // Üç bayt sýrasýyla gönderilir
    transmitSCIAChar(byte1);
    transmitSCIAChar(byte2);
    transmitSCIAChar(byte3);
}

//
// transmitSCIAChar - SCI'den bir karakter gönderir
//
void transmitSCIAChar(uint16_t a)
{
    while (SciaRegs.SCIFFTX.bit.TXFFST != 0)
    {

    }
    SciaRegs.SCITXBUF.all = a & 0xFF; // Alt 8 biti gönder
}

//
// initSCIAFIFO - SCI FIFO'yu baþlat
//
void initSCIAFIFO(void)
{
    SciaRegs.SCIFFTX.all = 0xE040;
    SciaRegs.SCIFFRX.all = 0x2044;
    SciaRegs.SCIFFCT.all = 0x0;
}


//
// cpuTimer0ISR - CPU Zamanlayýcý0 ISR kesme sayacý ile
//
__interrupt void cpuTimer0ISR(void)
{
    millisCounter++; // Her milisaniyede bir artýr

    // Eðer 1000 milisaniye (1 saniye) geçtiyse
    if (millisCounter % 1000 == 0)
    {
        // 32-bit deðiþkende saklanan 24-bit deðeri ilet
        uint32_t dataToSend = 0x00FFFFFF;
        transmitSCIADWord(dataToSend);

        loopCounter++;
    }

    CpuTimer0.InterruptCount++;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; // PIE kesme onayýný ver
}


//
// Dosyanýn Sonu
//
