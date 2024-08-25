/*
 * yaw_sampler.c
 *
 *  Created on: 15/04/2024
 *      Author: Lucas
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/pwm.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/debug.h"
#include "utils/ustdlib.h"
#include "yaw_sampler.h"

#define STEP_TO_DEGREE (360*10)/(112*4) //aka 8
#define HALF_TOTAL_STEPS (112*2)
#define TOTAL_STEPS (112*4)


//Pin map for phase A and B
#define QEI_A 0b00000001
#define QEI_B   0b00000010


//Helicopter yaw
static int32_t yaw = 0;

//State of phase A and B
static uint8_t encoder_state = 0;
//Next state of phase A and B
static uint8_t encoder_state_n = 0;

//falg for whether the yaw reference has been found
static uint8_t ref_flag = 0;

bool YawStatus = false;

//the various states the phase A and B encoder could be in
//in the form of decimal
//e.g. 0b00000001 = STATE_2
//     0b00000011 = STATE_3
enum QEI_STATES {
    STATE_1 = 0,
    STATE_2 = 1,
    STATE_3 = 3,
    STATE_4 = 2
};

//direction the encoder or heli is rotating
enum DIR {
    ANTICLOCKWISE = -1,
    NOT_MOVING = 0,
    CLOCKWISE = 1
};

void QEIIntHandler(void);
void ReferenceIntHandler(void);

//initialises the yaw reference pin interrupt
void initReferenceYaw()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)) {
    }
    GPIOIntRegister (GPIO_PORTC_BASE, ReferenceIntHandler);
    GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);

    GPIOIntTypeSet(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_FALLING_EDGE);

    GPIOIntEnable (GPIO_PORTC_BASE, GPIO_PIN_4);
}

//initialises the yaw encoder interrupt
void initYawEncoder()
{
    YawStatus = false;
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    GPIOPinTypeGPIOInput(GPIO_PORTB_BASE, QEI_A | QEI_B);
    GPIOIntTypeSet(GPIO_PORTB_BASE, QEI_A | QEI_B, GPIO_BOTH_EDGES);
    // Register the interrupt handler
    GPIOIntRegister(GPIO_PORTB_BASE, QEIIntHandler);
    GPIOIntDisable(GPIO_PORTB_BASE, QEI_A | QEI_B);
}

//Initialises all functions and variables related to
//the encoder
void initQEI (void)
{
    initReferenceYaw();
    initYawEncoder();

    yaw = HALF_TOTAL_STEPS;
    ref_flag = 0;

    QEIIntHandler();
}

//Calculates the direction of the encoder and returns
int8_t calc_direction()
{
    int8_t direction = NOT_MOVING;

    switch(encoder_state) {
    //compares encoder state with previous state to get direction
    case STATE_1:
        if(encoder_state_n == STATE_2) {
            direction = ANTICLOCKWISE;
        } else if(encoder_state_n == STATE_4) {
            direction = CLOCKWISE;
        }
        break;
    case STATE_2:
        if(encoder_state_n == STATE_3) {
            direction = ANTICLOCKWISE;
        } else if(encoder_state_n == STATE_1) {
            direction = CLOCKWISE;
        }
        break;
    case STATE_3:
        if(encoder_state_n == STATE_4) {
            direction = ANTICLOCKWISE;
        } else if (encoder_state_n == STATE_2) {
            direction = CLOCKWISE;
        }
        break;
    case STATE_4:
        if(encoder_state_n == STATE_1) {
            direction = ANTICLOCKWISE;
        } else if (encoder_state_n == STATE_3) {
            direction = CLOCKWISE;
        }
        break;
    }
    //if the state of the encoder doesn't change
    //then it ain't moving
    if(encoder_state == encoder_state_n) {
        direction = NOT_MOVING;
    }

    return direction;
}

// Adds the change in rotation to yaw
void sampleQEI(int8_t step_diff)
{
    yaw += step_diff;
    if(yaw >= TOTAL_STEPS) {
        yaw = 0;
    } else if(yaw < 0) {
        yaw = TOTAL_STEPS - 1;
    }
}

//Returns yaw
int_dp getQEISample()
{
    int_dp number;

    number.num = ((yaw - HALF_TOTAL_STEPS)*STEP_TO_DEGREE)/10;
    number.dp = abs(((yaw - HALF_TOTAL_STEPS)*STEP_TO_DEGREE)%10);
    number.steps = yaw - HALF_TOTAL_STEPS;

    return number;
}

//encoder interrupt handler
void QEIIntHandler(void)
{
    //sets encoder state to the next encoder state
    encoder_state = encoder_state_n;
    //sets next encoder state to the current encoder pins
    encoder_state_n = GPIOPinRead(GPIO_PORTB_BASE, QEI_A | QEI_B);
    //calculates the new yaw
    sampleQEI(calc_direction());

    GPIOIntClear(GPIO_PORTB_BASE, QEI_A | QEI_B);
}

//yaw reference interrupt handler
void ReferenceIntHandler(void)
{
    GPIOIntClear(GPIO_PORTC_BASE, GPIO_PIN_4);

    //normalises the yaw rotation to the centre
    //this results in yaw starting at 0 degrees after conversion
    yaw = HALF_TOTAL_STEPS;
    //sets reference flag
    ref_flag = 1;

    //enables the yaw encoder interrupt
    if(YawStatus == false) {
        YawStatus = true;
        GPIOIntEnable(GPIO_PORTB_BASE, QEI_A | QEI_B);
    }
    //disables the reference interrupt
    //GPIOIntDisable (GPIO_PORTC_BASE, GPIO_PIN_4);

}

//returns the reference flag to indicate to the
//fsm to change state
uint8_t getRefFlag() {
    return ref_flag;
}
