/*
 * PID_control.h
 *
 *  Created on: 5/05/2024
 *      Author: Lucas
 */

#ifndef PID_CONTROL_H_
#define PID_CONTROL_H_

//records all values required for a functional PID
typedef struct {
    int16_t setpoint;
    int16_t reading;
    int16_t Kp;
    int16_t Ki;
    int16_t Kd;
    int32_t I;
    int16_t prev_reading;
    int16_t duty_cycle;
} PID;

//initialises pins and variables associated with PID
void initPID(uint8_t frequency);

//either enables or disables the PWM for the main rotor
void MainPWMEnable(uint8_t enable);
//either enables or disables the PWM for the tail rotor
void TailPWMEnable(uint8_t enable);
//makes the integral sum or tail and main to 0
void eraseMainIntegral();
void eraseTailIntegral();

//uses current altitude and yaw as input
//set new duty cycle and updates PWM
void updateRotors(int16_t altitude, int16_t yaw);
//sets the target altitude setpoint
void setAltitude(int16_t altitude);
//sets the target yaw setpoint
void setYaw(int16_t yaw);

//gets duty cycle
uint16_t getMainDutyCycle();
uint16_t getTailDutyCycle();

#endif /* PID_CONTROL_H_ */
