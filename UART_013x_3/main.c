//
// Dahil Edilen Dosyalar
//
#include "f28x_project.h"

//
// Global De�i�kenler
//
uint16_t loopCounter = 0;

//
// Fonksiyon Prototipleri
//
void initSCIAEchoback(void);
void transmitSCIADWord(uint32_t data);
void initSCIAFIFO(void);
void transmitSCIAChar(uint16_t a);

// Fonksiyon Tan�mlamalar� GPIO SCI() GPIO28(RX)-GPIO29(TX)
void InitSciaGpio28_29(void);

//
// CPU Zamanlay�c�0
//

//
// Kesme Fonksiyonu Prototipleri
//
__interrupt void cpuTimer0ISR(void);

//
// millis A��klamas�
//
volatile unsigned long millisCounter = 0; // Milisaniye sayac�n� saklamak i�in de�i�ken

//
// Ana fonksiyon
//
void main(void)
{
    //
    // Cihaz saatini ve �evre birimlerini ba�lat
    //
    InitSysCtrl();

    //
    // GPIO28-GPIO29'u ba�lat
    //
    InitSciaGpio28_29();

    //
    // CPU kesmelerini devre d��� b�rak
    //
    DINT;

    //
    // PIE kontrol kay�tlar�n� varsay�lan durumlar�na ba�lat.
    // Varsay�lan durum, t�m PIE kesmelerinin devre d��� oldu�u ve bayraklar�n temizlendi�i durumdur.
    //
    InitPieCtrl();

    //
    // CPU kesmelerini devre d��� b�rak ve t�m CPU kesme bayraklar�n� temizle
    //
    IER = 0x0000;
    IFR = 0x0000;

    //
    // PIE vekt�r tablosunu kabuk Kesme Servis Rutinlerine (ISR) i�aret�ilerle ba�lat
    //
    InitPieVectTable();

    //
    // ISR fonksiyonlar�n� e�le
    //
    EALLOW;
    PieVectTable.TIMER0_INT = &cpuTimer0ISR;
    EDIS;

    //
    // Cihaz �evre birimlerini ba�lat. Bu �rnekte, sadece CPU Zamanlay�c�lar�n� ba�lat.
    //
    InitCpuTimers();

    //
    // CPU-Zamanlay�c� 0'� her milisaniyede bir kesme yapacak �ekilde yap�land�r:
    // 80MHz CPU Freq, 1 milisaniye Periyot (mikrosaniye cinsinden)
    //
    ConfigCpuTimer(&CpuTimer0, 80, 1000);

    //
    // Hassas zamanlama sa�lamak i�in, t�m kay�t defterine yazmak i�in sadece yazma
    // talimatlar�n� kullan. Bu nedenle, ConfigCpuTimer ve InitCpuTimers'ta
    // yap�land�rma bitlerinden herhangi biri de�i�irse, a�a��daki ayarlar�n da
    // g�ncellenmesi gerekir.
    //
    CpuTimer0Regs.TCR.all = 0x4000;

    //
    // CPU-Zamanlay�c� 0'a ba�l� olan CPU int1'i etkinle�tir.
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

    loopCounter = 0;

    initSCIAFIFO(); // SCI FIFO'yu ba�lat
    initSCIAEchoback(); // SCI'yi ekoyla geri g�nderim i�in ba�lat

    while(1)
    {
        // Ana d�ng� burada bo� b�rak�ld�, i�lem kesme i�inde yap�lacak
        // ��lemler her saniye kesme i�inde ger�ekle�ecek
    }
}

//
//  initSCIAEGPIO - GPIO28(RX)-GPIO29(TX)'i Ba�lat
//
void InitSciaGpio28_29(void)
{
    EALLOW;

    GpioCtrlRegs.GPAPUD.bit.GPIO28 = 0;  // GPIO28 i�in pull-up etkinle�tir (SCIRXDA)
    GpioCtrlRegs.GPAPUD.bit.GPIO29 = 0;  // GPIO29 i�in pull-up etkinle�tir (SCITXDA)

    GpioCtrlRegs.GPAQSEL2.bit.GPIO28 = 3;  // GPIO28 i�in asenkron giri� se� (SCIRXDA)

    GpioCtrlRegs.GPAMUX2.bit.GPIO28 = 1;  // GPIO28'i SCIRXDA i�lemi i�in yap�land�r
    GpioCtrlRegs.GPAMUX2.bit.GPIO29 = 1;  // GPIO29'u SCITXDA i�lemi i�in yap�land�r

    EDIS;
}


//
//  initSCIAEchoback - SCI-A'y� ekoyla geri g�nderim i�in ba�lat
//
void initSCIAEchoback(void)
{
    //
    // Not: SCIA �evre birimine saatler InitSysCtrl() fonksiyonunda a��ld�
    //
    SciaRegs.SCICCR.all = 0x0007;           // 1 stop bit, d�ng� yok
                                            // Parite yok, 8 karakter biti,
                                            // asenkron mod, bo�ta hat protokol�
    SciaRegs.SCICTL1.all = 0x0003;          // TX, RX, i� SCICLK etkin,
                                            // RX Hata yok, UYKU, TXWAKE devre d���
    SciaRegs.SCICTL2.all = 0x0003;
    SciaRegs.SCICTL2.bit.TXINTENA = 1;
    SciaRegs.SCICTL2.bit.RXBKINTENA = 1;

    //
    // 9600 baud'da SCIA
    // @LSPCLK = 25 MHz (100 MHz SYSCLK) HBAUD = 0x01  ve LBAUD = 0x44.
    //
    SciaRegs.SCIHBAUD.all = 0x0001;
    SciaRegs.SCILBAUD.all = 0x0044;

    SciaRegs.SCICTL1.all = 0x0023;          // SCI'yi Reset'ten ��kar
}

//
// transmitSCIADWord - 32-bit bir de�i�kende saklanan 24-bit de�eri iletir
//
void transmitSCIADWord(uint32_t data)
{
    uint16_t byte1, byte2, byte3;

    // 24-bit veri ��kart�l�r
    byte1 = (data >> 16) & 0xFF; // �st 8 bit
    byte2 = (data >> 8) & 0xFF;  // Orta 8 bit
    byte3 = data & 0xFF;         // Alt 8 bit

    // �� bayt s�ras�yla g�nderilir
    transmitSCIAChar(byte1);
    transmitSCIAChar(byte2);
    transmitSCIAChar(byte3);
}

//
// transmitSCIAChar - SCI'den bir karakter g�nderir
//
void transmitSCIAChar(uint16_t a)
{
    while (SciaRegs.SCIFFTX.bit.TXFFST != 0)
    {

    }
    SciaRegs.SCITXBUF.all = a & 0xFF; // Alt 8 biti g�nder
}

//
// initSCIAFIFO - SCI FIFO'yu ba�lat
//
void initSCIAFIFO(void)
{
    SciaRegs.SCIFFTX.all = 0xE040;
    SciaRegs.SCIFFRX.all = 0x2044;
    SciaRegs.SCIFFCT.all = 0x0;
}


//
// cpuTimer0ISR - CPU Zamanlay�c�0 ISR kesme sayac� ile
//
__interrupt void cpuTimer0ISR(void)
{
    millisCounter++; // Her milisaniyede bir art�r

    // E�er 1000 milisaniye (1 saniye) ge�tiyse
    if (millisCounter % 1000 == 0)
    {
        // 32-bit de�i�kende saklanan 24-bit de�eri ilet
        uint32_t dataToSend = 0x00FFFFFF;
        transmitSCIADWord(dataToSend);

        loopCounter++;
    }

    CpuTimer0.InterruptCount++;
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; // PIE kesme onay�n� ver
}


//
// Dosyan�n Sonu
//
