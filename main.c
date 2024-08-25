//*****************************************************************************
//
// ADCdemo1.c - Simple interrupt driven program which samples with AIN0
//
// Author:  Lucas
// Last modified:   8.2.2018
//
//*****************************************************************************
// Based on the 'convert' series from 2016
//*****************************************************************************

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
#include "circBufT.h"
#include "OrbitOLED/OrbitOLEDInterface.h"
#include "buttons4.h"
#include "stdio.h"
#include "stdlib.h"
#include "yaw_sampler.h"
#include "display.h"
#include "sampler.h"
#include "PID_control.h"
#include "switch.h"
#include "heliUART.h"
#include "reset.h"
#include "kernel.h"

//*****************************************************************************
// Constants
//*****************************************************************************

#define KERNEL_HZ 500

//defines the fsm states for readability
enum HeliStates {
    LANDED = 0,
    TAKE_OFF,
    HOVER,
    FLYING,
    LANDING,
};
enum HeliStates heli_state;

//conversions
#define ADC_TO_VOLTAGE 1241
#define DEGREE_TO_STEPS 112/90

#define TWO_SECOND_DELAY 6
#define MAX_KERNEL_TASKS 8
#define STARTUP_YAW_DUTY_CYCLE_OFFSET -30
#define MAX_ALTITUDE 100

//given thresholds for landing and take off purposes
#define ALTITUDE_THRESHOLD 4
#define YAW_THRESHOLD 5

//how much the yaw and alt change each button press
#define ALT_STEPS 10
#define YAW_STEPS 15

//how long it takes until main motor turns on after
//tail motor turns on
#define STARTUP_DELAY 10

//records adc values
uint16_t meanVal = 0;
uint16_t referenceVal = 0;
//records yaw stuff
int_dp yaw;

int16_t altitude_percent = 0;
uint8_t butState[4], switchState;

//records time until START_UP_DELAY to turn on main rotor
int16_t initTimer = 0;

int16_t heliAltitude = 0;
int16_t heliYaw = 0;

//holds info that will be displayed onto oled
uart_info statusInfo;


//records the id of tasks registered in the kernel
uint8_t iDTriggerADC = 0;
uint8_t iDTriggerUpdate = 0;
uint8_t iDTriggerUpdateRotors = 0;
uint8_t iDTriggerSetYaw = 0;
uint8_t iDTriggerSetAltitude = 0;
uint8_t iDTriggerUART = 0;
uint8_t iDTriggerDisplayOLED = 0;


//converts raw ADC value to percent
int16_t valToPercent(uint16_t referenceVal, uint16_t meanVal)
{
    return (int16_t)(referenceVal - meanVal)*100 / ADC_TO_VOLTAGE;
}

//triggers adc
void TriggerADC()
{
    ADCProcessorTrigger(ADC0_BASE, 3);
}

//displays stuff on the oled display
void TriggerDisplayOLED()
{
    displayOLED(altitude_percent, yaw, getMainDutyCycle(), getTailDutyCycle());
}

//makes uart send stuff over serial
void TriggerUART()
{
    statusInfo.actual_alt = altitude_percent;
    statusInfo.actual_yaw = yaw.num;
    statusInfo.desired_alt = heliAltitude;
    statusInfo.desired_yaw = heliYaw;
    statusInfo.main_duty_cycle = getMainDutyCycle();
    statusInfo.tail_duty_cycle = getTailDutyCycle();
    statusInfo.operating_mode = heli_state;

    //makes the stuff above be sent via uart
    sendMessage(&statusInfo);
}

//updates motor value and pwm/pid
void TriggerUpdate()
{
    meanVal = getSample();
    yaw = getQEISample();
    updateRotors(referenceVal - meanVal, yaw.steps);
}

//Sets the altitude setpoint
void TriggerSetAltitude()
{
    setAltitude(heliAltitude*ADC_TO_VOLTAGE/100);
    //stops function being scheduled
    K_block_task(iDTriggerSetAltitude);
}

//Sets the yaw setpoint
void TriggerSetYaw()
{
    setYaw(heliYaw*DEGREE_TO_STEPS);
    //stops function being scheduled
    K_block_task(iDTriggerSetYaw);
}

//updates swiytch and button inputs
void AltUpdate()
{
    altitude_percent = valToPercent(referenceVal, meanVal);
}

void InputUpdate()
{
    updateSwitch();
    switchState = checkSwitch();

    updateButtons();
    butState[UP] = checkButton (UP);
    butState[DOWN] = checkButton (DOWN);
    butState[LEFT] = checkButton (LEFT);
    butState[RIGHT] = checkButton (RIGHT);
}

//--------------------------------
//      FSM Stuff START
void FSMLanded()
{
    referenceVal = getSample();
    //if switched up then start takeoff mode
    //enable tail first so heli doesn't spin up as much
    if(switchState == S_UP) {
        TailPWMEnable(1);
        heliYaw = 0;
        initTimer = 0;
        K_ready_task(iDTriggerSetYaw);
        //remove windup
        eraseMainIntegral();
        heli_state = TAKE_OFF;
    }
}

void FSMTakeoff()
{
    //starts hover mode when at chosen height
    //will rotate until it finds the yaw reference
    if(initTimer == -1 && abs(altitude_percent - heliAltitude) <= ALTITUDE_THRESHOLD) {
        heli_state = HOVER;
        heliYaw = STARTUP_YAW_DUTY_CYCLE_OFFSET;
        K_ready_task(iDTriggerSetYaw);
    }
    //after a certain time turn on main motor
    if(initTimer >= STARTUP_DELAY) {
        //remove windup
        eraseMainIntegral();
        MainPWMEnable(1);
        heliAltitude = ALT_STEPS;
        K_ready_task(iDTriggerSetAltitude);
        //set initTimer to -1 to prevent this being called again
        initTimer = -1;
    } else if(initTimer != -1){
        ++initTimer;
    }
}

void FSMHover()
{
    //removes yaw windup
    eraseTailIntegral();
    //if reference found then go to controllable flying mode
    if(getRefFlag() == 1) {
        K_ready_task(iDTriggerSetYaw);
        heliYaw = 0;
        heli_state = FLYING;
    }
}

void FSMFlying()
{
    //moves helicopter up 10% when up button pressed unless its at max height
    if(butState[UP] == PUSHED) {
        heliAltitude += ALT_STEPS;
        if(heliAltitude > MAX_ALTITUDE) {
            heliAltitude = MAX_ALTITUDE;
        }
        K_ready_task(iDTriggerSetAltitude);
    }
    //moves helicopter down 10% when down button pressed unless its at min height
    if(butState[DOWN] == PUSHED) {
        heliAltitude -= ALT_STEPS;
        if(heliAltitude < 0) {
            heliAltitude = 0;
        }
        K_ready_task(iDTriggerSetAltitude);
    }
    //moves helicopter clockwise 15 degrees, makes sure helicopter doesnt flip round
    // when it passes outside of bounds
    if(butState[RIGHT] == PUSHED) {
        if(heliYaw >= 180) {
            heliYaw = -180;
        }
        heliYaw += YAW_STEPS;
        K_ready_task(iDTriggerSetYaw);
    }
    //moves helicopter counter clockwise 15 degrees, makes sure helicopter doesnt flip round
    // when it passes outside of bounds
    if(butState[LEFT] == PUSHED) {
        if(heliYaw <= -180+YAW_STEPS) {
            heliYaw = 180+YAW_STEPS;
        }
        heliYaw -= YAW_STEPS;
        K_ready_task(iDTriggerSetYaw);
    }
    //initiates landing mode when switch it switched down
    if(switchState == S_DOWN) {
        heli_state = LANDING;
        heliYaw = 0;
        K_ready_task(iDTriggerSetYaw);
    }
}

//landing process
void FSMLanding()
{
    //waits until alt drops 10 before dropping more
    //ensures smooth landing
    if(abs(yaw.num) <= YAW_THRESHOLD && altitude_percent <= heliAltitude + ALTITUDE_THRESHOLD && heliAltitude > 0) {
        heliAltitude -= ALT_STEPS;
        K_ready_task(iDTriggerSetAltitude);
    }
    //once landed turn off motors
    if(abs(yaw.num) <= YAW_THRESHOLD && altitude_percent <= heliAltitude + ALTITUDE_THRESHOLD && heliAltitude == 0) {
        heli_state = LANDED;
        MainPWMEnable(0);
        TailPWMEnable(0);
    }
}

//Defines FSM
void finite_state_machine()
{
    //updates buttions and switches
    InputUpdate();
    //does fsm stuff
    switch(heli_state) {
    case LANDED:
        FSMLanded();
        break;
    case TAKE_OFF:
        FSMTakeoff();
        break;
    case HOVER:
        FSMHover();
        break;
    case FLYING:
        FSMFlying();
        break;
    case LANDING:
        FSMLanding();
        break;
    }
}

//      FSM Stuff End
//---------------------------

//initialises everything
void initialise()
{

    IntMasterDisable();

    K_init(MAX_KERNEL_TASKS, KERNEL_HZ); //sets maks tasks and kernel rate
    initADC ();
    initPID(60);
    initQEI();
    initDisplay ();
    initialiseUSB_UART();
    initButtons ();
    initSwitch();
    initReset();

    //registers functions as tasks alongside their frequency and priority
    iDTriggerADC = K_register_task(TriggerADC, 120, 0);
    iDTriggerUpdate = K_register_task(TriggerUpdate, 60, 1);
    K_register_task(AltUpdate, 60, 2);
    K_register_task(finite_state_machine, 60, 3);
    iDTriggerSetYaw = K_register_task(TriggerSetYaw, 20, 4);
    iDTriggerSetAltitude = K_register_task(TriggerSetAltitude, 20, 5);
    iDTriggerUART = K_register_task(TriggerUART, 4, 6);
    iDTriggerDisplayOLED = K_register_task(TriggerDisplayOLED, 4, 7);

    //blocks as defult
    K_block_task(iDTriggerSetYaw);
    K_block_task(iDTriggerSetAltitude);

    //sets defult values
    switchState = S_DOWN;
    heli_state = LANDED;
    heliAltitude = 0;
    heliYaw = 0;
    initTimer = 0;

    statusInfo.actual_alt = 0;
    statusInfo.actual_yaw = 0;
    statusInfo.desired_alt = 0;
    statusInfo.desired_yaw = 0;
    statusInfo.main_duty_cycle = 0;
    statusInfo.tail_duty_cycle = 0;


    IntMasterEnable();

    //delays for two seconds to allow for things to enable properly
    SysCtlDelay (SysCtlClockGet() / TWO_SECOND_DELAY);

    //disables motors
    MainPWMEnable(0);
    TailPWMEnable(0);
}

int
main(void)
{
    //initialises stuff
    initialise();
    //starts scheduler
    while (1)
    {
        K_start();
    }
}

