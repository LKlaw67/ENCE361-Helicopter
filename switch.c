/*
 * switch_1.c
 *
 *  Created on: 6/05/2024
 *      Author: Lucas
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/debug.h"
#include "inc/tm4c123gh6pm.h"
#include "switch.h"

//number of times polled to prevent bouncing
#define NUM_POLLS 3

static bool state = 1;    // Corresponds to the electrical state
static uint8_t count = 0;
static bool switch_flag = false;

//initalises and configures pins/ports
void initSwitch()
{

    SysCtlPeripheralEnable (SYSCTL_PERIPH_GPIOA);
    GPIOPinTypeGPIOInput (GPIO_PORTA_BASE, GPIO_PIN_7);
    GPIOPadConfigSet (GPIO_PORTA_BASE, GPIO_PIN_7, GPIO_STRENGTH_2MA,
                      GPIO_PIN_TYPE_STD_WPD);
}

//updates switches
void updateSwitch()
{
    bool switch_value;
    switch_value = (GPIOPinRead (GPIO_PORTA_BASE, GPIO_PIN_7) == GPIO_PIN_7);

    //does debouncing
    if (switch_value != state)
    {
        count++;
        if (count >= NUM_POLLS)
        {
            //once at a state long enough, return current state
            state = switch_value;
            switch_flag = true;    // Reset by call to checkButton()
            count = 0;
        }
    }
    else {
        count = 0;
    }
}

//checks if switch has changed position
uint8_t checkSwitch()
{
    if (switch_flag)
    {
        switch_flag = false;
        if (state == 1)
            return S_UP;
        else
            return S_DOWN;
    }
    return S_NO_CHANGE;
}
