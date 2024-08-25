/*
 * display.c
 *
 *  Created on: 16/03/2024
 *      Author: Lucas
 */

#include <stdint.h>
#include "OrbitOLED/OrbitOLEDInterface.h"
#include "stdlib.h"
#include "utils/ustdlib.h"
#include "buttons4.h"
#include "yaw_sampler.h"

#define NUM_DISPLAY_STATE 2
#define MAX_STR_LEN 17

//corresponds to display lines
enum DISPLAY_HEIGHT {
    LINE_0 = 0,
    LINE_1,
    LINE_2,
    LINE_3
};

void
initDisplay (void)
{
    // intialise the Orbit OLED display
    OLEDInitialise ();
}

//displays altitude, yaw, main rotor duty cycle, tail rotor duty cycle
void displayOLED(int16_t altitude, int_dp yaw, uint16_t main_duty_cycle, uint16_t tail_duty_cycle)
{
    char string[MAX_STR_LEN];

    
    usnprintf (string, sizeof(string), "Alt  = %4d%%", altitude);  //creates string that will be displayed
    OLEDStringDraw ("Alt  =          ", 0, LINE_0); //clears only the number
    OLEDStringDraw (string, 0, LINE_0); //draws to display

    usnprintf (string, sizeof(string), "Yaw  = %3d.%d", yaw.num, yaw.dp); //creates string that will be displayed
    OLEDStringDraw ("Yaw  =          ", 0, LINE_1); //clears only the number
    OLEDStringDraw (string, 0, LINE_1);//draws to display

    usnprintf (string, sizeof(string), "Main = %4d", main_duty_cycle); //creates string that will be displayed
    OLEDStringDraw ("Main =     ", 0, LINE_2); //clears only the number
    OLEDStringDraw (string, 0, LINE_2);//draws to display

    usnprintf (string, sizeof(string), "Tail = %4d", tail_duty_cycle); //creates string that will be displayed
    OLEDStringDraw ("Tail =     ", 0, LINE_3); //clears only the number
    OLEDStringDraw (string, 0, LINE_3);//draws to display
}
