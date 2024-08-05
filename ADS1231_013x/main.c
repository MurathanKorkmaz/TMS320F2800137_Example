
#include "f28x_project.h"

#define SCLK    3   // OUTPUT   1 GPIO3 (Clock Signal for ADS)
#define DOUT    1   // INPUT    0 GPIO1 (Data Output from ADS)
#define SPEED   0   // OUTPUT   0 GPIO0 (Speed Selection for ADS)
#define PDWN    40  // OUTPUT   1 GPIO40 (Power Down Control for ADS)

// Function Prototypes
void Defines_Pin_ADS();
void ADS_begin(int highSpeed, int power);
int ADS_read();

// Global variables
volatile unsigned long millisCounter = 0; // Counter to store milliseconds
uint32_t val = 0; // Variable to store the read value from ADS
int maskResult = 0; // Variable to store the result of ADS_read function

// Interrupt Service Routine (ISR) Prototypes
__interrupt void cpuTimer0ISR(void);

// millis Function Prototype
unsigned long millis(void);

void main(void)
{
    // Initialize the device clock and peripherals
    InitSysCtrl();

    // Initialize GPIO and configure the GPIO pins
    InitGpio();

    Defines_Pin_ADS(); // Configure GPIO pins for ADS
    ADS_begin(0, 1); // Initialize ADS with normal speed and power on

    DINT; // Disable CPU interrupts
    InitPieCtrl(); // Initialize PIE control registers
    IER = 0x0000; // Disable all CPU interrupts
    IFR = 0x0000; // Clear all CPU interrupt flags

    // Initialize the PIE vector table with pointers to the shell ISR
    InitPieVectTable();

    // Map ISR functions
    EALLOW;
    PieVectTable.TIMER0_INT = &cpuTimer0ISR; // Map Timer0 ISR
    EDIS;

    // Initialize the Device Peripherals. For this example, only initialize the CPU Timers.
    InitCpuTimers();

    // Configure CPU-Timer 0 to interrupt every millisecond:
    // 80MHz CPU Freq, 1 millisecond Period (in microseconds)
    ConfigCpuTimer(&CpuTimer0, 80, 1000);

    // In case of any changes to CPU-Timer configuration bits
    // in ConfigCpuTimer and InitCpuTimers, update these settings accordingly.
    CpuTimer0Regs.TCR.all = 0x4000; // Start Timer0

    // Enable CPU int1 which is connected to CPU-Timer 0
    IER |= M_INT1;

    // Enable TINT0 in the PIE: Group 1 interrupt 7
    PieCtrlRegs.PIEIER1.bit.INTx7 = 1;

    // Enable Global Interrupt (INTM) and real-time interrupt (DBGM)
    EINT;
    ERTM;

    while(1)
    {
        static unsigned long previousMillis = 0;
        unsigned long currentMillis = millis(); // Get current time in milliseconds

        // Check if 100 milliseconds have passed
        if(currentMillis - previousMillis >= 100)
        {
            previousMillis = currentMillis;
            maskResult = ADS_read(); // Read data from ADS
            // The actual value is stored in the global variable 'val'
        }
    }
}

void Defines_Pin_ADS()
{
    // Configure digital INPUT
    // Setup GPIO1 as digital input with pull-down resistor
    GPIO_SetupPinMux(DOUT, GPIO_MUX_CPU1, 0); // Set pin function to GPIO
    GPIO_SetupPinOptions(DOUT, GPIO_INPUT, 0); // Configure as input, enable pull-down resistor

    // Configure digital OUTPUT
    // Setup GPIO0, GPIO3, and GPIO40 as digital output
    GPIO_SetupPinMux(SPEED, GPIO_MUX_CPU1, 0); // Set pin function to GPIO
    GPIO_SetupPinOptions(SPEED, GPIO_OUTPUT, GPIO_PUSHPULL); // Configure as output, push-pull

    GPIO_SetupPinMux(SCLK, GPIO_MUX_CPU1, 0); // Set pin function to GPIO
    GPIO_SetupPinOptions(SCLK, GPIO_OUTPUT, GPIO_PUSHPULL); // Configure as output, push-pull

    GPIO_SetupPinMux(PDWN, GPIO_MUX_CPU1, 0); // Set pin function to GPIO
    GPIO_SetupPinOptions(PDWN, GPIO_OUTPUT, GPIO_PUSHPULL); // Configure as output, push-pull
}

void ADS_begin(int highSpeed, int power)
{
    // Set the speed of ADS
    if(highSpeed)
    {
        GPIO_WritePin(SPEED, 1); // High speed mode
    }
    else
    {
        GPIO_WritePin(SPEED, 0); // Normal speed mode
    }

    // Set the power mode of ADS
    if(power)
    {
        GPIO_WritePin(PDWN, 1); // Power on
    }
    else
    {
        GPIO_WritePin(PDWN, 0); // Power down
    }

    GPIO_WritePin(DOUT, 1); // Initialize DOUT to high
    GPIO_WritePin(SCLK, 0); // Initialize SCLK to low
}

int ADS_read()
{
    int i = 0;
    unsigned long start;
    int startOffset;

    // Determine start offset based on speed
    if(GPIO_ReadPin(SPEED) == 1)
    {
        startOffset = 19; // Offset for high speed
    }
    else
    {
        startOffset = 150; // Offset for normal speed
    }

    start = millis(); // Get start time
    // Wait for DOUT to go high
    while(GPIO_ReadPin(DOUT) != 1)
    {
        if(millis() > start + startOffset) // Timeout condition
        {
            return -1;
        }
    }

    start = millis(); // Reset start time
    // Wait for DOUT to go low
    while(GPIO_ReadPin(DOUT) != 0)
    {
        if(millis() > start + startOffset) // Timeout condition
        {
            return -2;
        }
    }

    // Read 24 bits from ADS
    for(i = 23; i >= 0; i--)
    {
        GPIO_WritePin(SCLK, 1); // Set clock high
        val = (val << 1) + GPIO_ReadPin(DOUT); // Read bit and shift into val
        GPIO_WritePin(SCLK, 0); // Set clock low
    }

    val = (val << 8) / 256; // Convert to 24-bit value

    GPIO_WritePin(SCLK, 1); // Extra clock pulse
    GPIO_WritePin(SCLK, 0);

    return 0; // Return success
}

__interrupt void cpuTimer0ISR(void)
{
    millisCounter++; // Increment the millisecond counter

    CpuTimer0.InterruptCount++; // Increment interrupt count
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1; // Acknowledge the PIE group 1 interrupt
}

unsigned long millis(void)
{
    return millisCounter; // Return the number of milliseconds since start
}
