/*
 * kernel.c
 *
 *  Created on: 11/05/2024
 *      Author: Lucas
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

//address to the systick priority
//should have secondary priority to yaw and adc interrupts
#define SYSTICK_PRIORITY    (*((volatile uint32_t *)0xE000ED20))

//defines tasks
typedef struct {
    void (*Task)(void);
    uint8_t priority;
    bool flag;
    bool ready;
    uint16_t time;
    uint8_t frequency;
} TASKS;

//assigns values to tasks
static TASKS* tasks;
static uint8_t* priority_order;
static uint8_t index = 0;
static uint32_t clockCounter = 0;
static uint16_t SysTickFreq = 0;

//goes through tasks in order of priority
void
SysTickIntHandler(void)
{
    //checks every tasks in priority order
    uint8_t i = 0;
    for(i = 0; i < index; ++i) {
        //only executes if the tasks is ready
        if(tasks[priority_order[i]].ready == true) {
            if(clockCounter >= tasks[priority_order[i]].time) {
                //sets true flag to be executed
                tasks[priority_order[i]].flag = true;
                //shifts time for next execution
                tasks[priority_order[i]].time += (SysTickFreq / tasks[priority_order[i]].frequency);
            }
        }
    }
    ++clockCounter;
}

//*****************************************************************************
// Initialisation functions for the clock (incl. SysTick), ADC, display
//*****************************************************************************
void
initClock (unsigned long tickFrequency)
{
    SysTickFreq = tickFrequency;
    SysCtlClockSet (SYSCTL_SYSDIV_10 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                   SYSCTL_XTAL_16MHZ);
    //
    // Set up the period for the SysTick timer.  The SysTick timer period is
    // set as a function of the system clock.
    SysTickPeriodSet(SysCtlClockGet() / SysTickFreq);
    //
    // Register the interrupt handler
    SysTickIntRegister(SysTickIntHandler);
    //Set priority to 1
    SYSTICK_PRIORITY = (SYSTICK_PRIORITY & 0x00FFFFFF) | 0x20000000;
    // Enable interrupt
    SysTickIntEnable();
    SysTickEnable();
}

//makes sure the number of task is not above the max
void K_init(uint8_t maxTasks, unsigned long tickFrequency)
{
    index = 0;

    initClock(tickFrequency);

    //allocates memory according to max tasks
    tasks = malloc(maxTasks * sizeof(TASKS));
    priority_order = malloc(maxTasks * sizeof(uint8_t));
}

//sets initial values and states for the tasks struct
unsigned char K_register_task(void (*taskEnter) (void), uint16_t frequency, unsigned char priority)
{
    tasks[index].Task = taskEnter;
    tasks[index].priority = priority;
    tasks[index].flag = false;
    tasks[index].ready = true;
    tasks[index].frequency = frequency;
    tasks[index].time = 0;
    priority_order[priority] = index;

    ++index;

    return index - 1;
}

//sets the task id to unassigned
void K_unregister_task(unsigned char taskId)
{
    tasks[taskId].Task = NULL;
}

//sets task ready to be scheduled
void K_ready_task(unsigned char taskId)
{
    tasks[taskId].ready = true;
}
//sets tasks not able to be scheduled
void K_block_task(unsigned char taskId)
{
    tasks[taskId].ready = false;
}
//sets task state to ready
uint8_t K_task_state(unsigned char taskId)
{
    return tasks[taskId].ready;
}
//starts time triggered scheduler
void K_start(void)
{
    //executres tasks in priority order
    uint8_t a = 0;
    for(a = 0; a < index; ++a) {
        if(tasks[priority_order[a]].flag == true) {
            tasks[priority_order[a]].flag = false;
            tasks[priority_order[a]].Task();
        }
    }
}
