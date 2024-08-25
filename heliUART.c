/*
 * heliUART.c
 *
 *  Created on: 7/05/2024
 *      Author: jlf78
 */

#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/debug.h"
#include "driverlib/pin_map.h"
#include "utils/ustdlib.h"
#include "stdio.h"
#include "stdlib.h"
#include "OrbitOLED/OrbitOLEDInterface.h"
#include "heliUART.h"

//uart definitions
#define BAUD_RATE 9600
#define UART_USB_BASE           UART0_BASE
#define UART_USB_PERIPH_UART    SYSCTL_PERIPH_UART0
#define UART_USB_PERIPH_GPIO    SYSCTL_PERIPH_GPIOA
#define UART_USB_GPIO_BASE      GPIO_PORTA_BASE
#define UART_USB_GPIO_PIN_RX    GPIO_PIN_0
#define UART_USB_GPIO_PIN_TX    GPIO_PIN_1
#define UART_USB_GPIO_PINS      UART_USB_GPIO_PIN_RX | UART_USB_GPIO_PIN_TX
#define MAX_STR_LEN 72
#define MODE_STR_LEN 8

//status string that will be sent over serial
char statusStr[MAX_STR_LEN + 1];

//defines states for helicopter
enum HeliStates {
    LANDED = 0,
    TAKE_OFF,
    HOVER,
    FLYING,
    LANDING,
};

//inits UART
void
initialiseUSB_UART (void)
{
    //
    // Enable GPIO port A which is used for UART0 pins.
    //
    SysCtlPeripheralEnable(UART_USB_PERIPH_UART);
    SysCtlPeripheralEnable(UART_USB_PERIPH_GPIO);
    //
    // Select the alternate (UART) function for these pins.
    //
    GPIOPinTypeUART(UART_USB_GPIO_BASE, UART_USB_GPIO_PINS);
    GPIOPinConfigure (GPIO_PA0_U0RX);
    GPIOPinConfigure (GPIO_PA1_U0TX);

    UARTConfigSetExpClk(UART_USB_BASE, SysCtlClockGet(), BAUD_RATE,
            UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
            UART_CONFIG_PAR_NONE);
    UARTFIFOEnable(UART_USB_BASE);
    UARTEnable(UART_USB_BASE);
}

void
UARTSend (char *pucBuffer)
{
    // Loop while there are more characters to send.
    while(*pucBuffer)
    {
        // Write the next character to the UART Tx FIFO.
        UARTCharPut(UART_USB_BASE, *pucBuffer);
        pucBuffer++;
    }
}

//sends UART message
void sendMessage(uart_info* info)
{
    //sets string to mode str length
    char mode[MODE_STR_LEN];
    //sets mode to the current state
    switch(info->operating_mode) {
    case LANDED:
        usnprintf (mode, sizeof(mode), "LANDED ");
        break;
    case TAKE_OFF:
        usnprintf (mode, sizeof(mode), "TAKEOFF");
        break;
    case HOVER:
        usnprintf (mode, sizeof(mode), "HOVER  ");
        break;
    case FLYING:
        usnprintf (mode, sizeof(mode), "FLYING ");
        break;
    case LANDING:
        usnprintf (mode, sizeof(mode), "LANDING");
        break;
    }

    //formats statusStr for readability
   usprintf (statusStr, "Yaw: %2d[%2d]\r\nAlt: %2d[%2d]\r\nMain: %2d Yaw: %2d\r\nMode: %s\r\n-----\r\n",
              info->desired_yaw, info->actual_yaw, info->desired_alt, info->actual_alt, info->main_duty_cycle, info->tail_duty_cycle, mode);
   //sends
    UARTSend (statusStr);
}
