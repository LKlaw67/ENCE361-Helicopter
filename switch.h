/*
 * switch_1.h
 *
 *  Created on: 6/05/2024
 *      Author: Lucas
 */

#ifndef SWITCH_H_
#define SWITCH_H_

enum switchStates {S_UP = 0, S_DOWN, S_NO_CHANGE};

void initSwitch();
void updateSwitch();
uint8_t checkSwitch();

#endif /* SWITCH_H_ */
