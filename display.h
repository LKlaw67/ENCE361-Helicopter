/*
 * display.h
 *
 *  Created on: 16/03/2024
 *      Author: Lucas
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>

//Initializes display
void
initDisplay (void);

//cycles through display states
void cycleDisplay();
int16_t valToPercent(uint16_t referenceVal, uint16_t meanVal);

//displays information
void displayOLED(uint16_t altitude, int_dp yaw, uint16_t main_duty_cycle, uint16_t tail_duty_cycle);

#endif /* DISPLAY_H_ */
