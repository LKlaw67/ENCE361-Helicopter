/*
 * kernal.h
 *
 *  Created on: 11/05/2024
 *      Author: Lucas
 */

#ifndef KERNEL_H_
#define KERNEL_H_

//inits kernel and sets max tasks and frequency
void K_init(uint8_t maxTasks, unsigned long tickFrequency);

//records a pointer to the task function
//records the frequency at which the task will be executed
//sets priority order in which it will be executed
unsigned char K_register_task(void (*taskEnter) (void), uint16_t frequency, unsigned char priority);

//starts time triggered scheduler
void K_start(void);

void K_unregister_task(unsigned char taskId);//unregisters task
void K_ready_task(unsigned char taskId);//readies task to be executed
void K_block_task(unsigned char taskId);//stops task from being scheduled
uint8_t K_task_state(unsigned char taskId);//returns task state

#endif /* KERNEL_H_ */
