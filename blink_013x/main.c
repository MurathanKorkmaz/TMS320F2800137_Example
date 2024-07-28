//

#include "f28x_project.h"


#define DEVICE_GPIO_PIN_LED1    24

void digitalWrite(int code, int value);

void main(void)
{

    InitSysCtrl();

    EALLOW;
    GpioCtrlRegs.GPAMUX1.bit.GPIO11=0;
    GpioCtrlRegs.GPADIR.bit.GPIO11=1;
    EDIS;

    while(1)
    {
        digitalWrite(11,0);
        DELAY_US(100000);
        digitalWrite(11,1);
        DELAY_US(100000);
    }
}

void digitalWrite(int code, int value)
{
    if(code == 11)
    {
        if(value == 0)
        {
            GpioDataRegs.GPACLEAR.bit.GPIO11 = 1;
        }
        else if(value == 1)
        {
            GpioDataRegs.GPASET.bit.GPIO11 = 1;
        }
    }
}
