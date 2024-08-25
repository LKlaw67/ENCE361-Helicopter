/*
 * yaw_sampler.h
 *
 *  Created on: 15/04/2024
 *      Author: Lucas
 */

#ifndef YAW_SAMPLER_H_
#define YAW_SAMPLER_H_

//used to store yaw info
//stores num = absolute degrees
//       dp = decimal point degrees
//       steps = encoder steps
typedef struct INT_DP {
    int16_t num;
    uint8_t dp;
    int16_t steps;
} int_dp;

//Initialises everything required for the QEI
//including setting the initial yaw rotation
void initQEI (void);

//Returns the current yaw rotation of the helicopter
//relative to the initialised position
int_dp getQEISample();


//returns a true flag if the yaw reference is found
uint8_t getRefFlag();

#endif /* YAW_SAMPLER_H_ */
