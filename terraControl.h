// ==================================== [terraControl.h] =============================
/*
*	This include file defines the data sructures for the TerraControl unit.
*
*	Author: Tobias Braechter
*	Last update: 2020-06-21
*
*/

#ifndef _TERRA_CONTROL_H_
	#define _TERRA_CONTROL_H_ 1

	#include <stdint.h>

	#define NUM_OPT 17
	#define OPT_NONE 0
	#define OPT_LIGHT 1
	#define OPT_DAY_HOUR 2
	#define OPT_DAY_MIN 3
	#define OPT_DAY_RED 4
	#define OPT_DAY_GRE 5
	#define OPT_DAY_BLU 6
	#define OPT_DAY_TEMP 7
	#define OPT_NIGHT_HOUR 8
	#define OPT_NIGHT_MIN 9
	#define OPT_NIGHT_RED 10
	#define OPT_NIGHT_GRE 11
	#define OPT_NIGHT_BLU 12
	#define OPT_NIGHT_TEMP 13
	#define OPT_HOUR 14
	#define OPT_MIN 15
	#define OPT_CLOCK 16

	#define NUM_DAT 6
	#define DAT_OPTION 0
	#define DAT_TEMP 1
	#define DAT_TEMP_OK 2
	#define DAT_HYGRO 3
	#define DAT_HYGRO_OK 4
	#define DAT_DAYTIME 5

	#define MAX_NONE 1
	#define MAX_LIGHT 2
	#define MAX_HOUR 23
	#define MAX_MIN 59
	#define MAX_SEC 59
	#define MAX_PWM 100
	#define MAX_TEMP 50
	#define MAX_CLOCK 200

	#define OPT_LIGHT_AUTO 0
	#define OPT_LIGHT_ON 1
	#define OPT_LIGHT_OFF 2

	#define DAYTIME_DAY 0
	#define DAYTIME_NIGHT 1

	uint8_t options[NUM_OPT]; // current value of each option
	uint8_t optionMax[NUM_OPT]; // maximum value of each option
	uint8_t optionsCache[NUM_OPT]; // cache value of each option
	uint8_t data[NUM_DAT]; // current data values
	uint8_t dataCache[NUM_DAT]; // cache data values

#endif
