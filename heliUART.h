/*
 * heliUART.h
 *
 *  Created on: 7/05/2024
 *      Author: jlf78
 */

#ifndef HELIUART_H_
#define HELIUART_H_

typedef struct UART_INFO {
    int16_t desired_yaw;
    int16_t actual_yaw;
    int16_t desired_alt;
    int16_t actual_alt;
    int16_t main_duty_cycle;
    int16_t tail_duty_cycle;
    uint8_t operating_mode;
} uart_info;

//takes the info above as an input
//formats the info into readable text and
//sends data to terminal
void sendMessage(uart_info* info);

//initialises UART and variables
void initialiseUSB_UART (void);

#endif /* HELIUART_H_ */
