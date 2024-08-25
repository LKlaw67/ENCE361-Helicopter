/*
 * reset.c
 *
 *  Created on: 7/05/2024
 *      Author: Lucas
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "inc/hw_ints.h"
#include "driverlib/adc.h"
#include "driverlib/pwm.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "utils/ustdlib.h"

void resetIntHandler();

//Initializes reset interrupt
void initReset()
{

    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOA);
    GPIOIntRegister (GPIO_PORTA_BASE, resetIntHandler);
    GPIOPinTypeGPIOInput (GPIO_PORTA_BASE, GPIO_PIN_6);
    GPIOPadConfigSet (GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
    GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_FALLING_EDGE);
    GPIOIntEnable (GPIO_PORTA_BASE, GPIO_PIN_6);
}

//when executed resets tiva board
void resetIntHandler()
{
    GPIOIntClear(GPIO_PORTA_BASE, GPIO_PIN_6);
    SysCtlReset();
}
