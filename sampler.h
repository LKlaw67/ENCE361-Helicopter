/*
 * sampler.h
 *
 *  Created on: 17/03/2024
 *      Author: Lucas
 */

#ifndef SAMPLER_H_
#define SAMPLER_H_

//inits ADC sampler
void
initADC (void);

//returns the current ADC value
uint16_t getSample();

#endif /* SAMPLER_H_ */
