// ==================================== [display.h] =============================
/*
*	This include file defines functions to control a 2.8in display on an Atmel ATmega.
*
*	Author: Tobias Braechter
*	Last update: 2020-06-21
*
*/

#ifndef _DISPLAY_H_
	#define _DISPLAY_H_ 1

	#include <avr/io.h>
	#include <stdint.h>

	#include "bitOperation.h"

	#define DDR_DISP_SEL &DDRC,0
	#define DDR_DISP_READ &DDRC,1
	#define DDR_DISP_WRITE &DDRC,2
	#define DISP_SEL &PORTC,0
	#define DISP_READ &PORTC,1
	#define DISP_WRITE &PORTC,2

	#define DDR_DISP_DATA DDRB
	#define DISP_DATA_OUT PORTB
	#define DISP_DATA_IN PINB

	#define DISP_SEL_COM 0
	#define DISP_SEL_DAT 1

	#define DISP_MAX_X 319
	#define DISP_MAX_Y 239

	uint8_t dispColor[3]; // buffer for display color

	uint16_t xStart, yStart, xEnd, yEnd; // store coordinates for calculations
	uint16_t charX;

	void dispWrite(uint8_t sel, uint8_t data); // write data to display
	void setColor(uint8_t red, uint8_t green, uint8_t blue); // set the drawing color
	void storePosition(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2); // store position into local memory
	void drawPoint(uint16_t x, uint16_t y); // draw a point on display
	void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2); // draw a line on display
	void drawRect(uint16_t x, uint16_t y, uint16_t sizeX, uint16_t sizeY); // draw a rectangle on display
	void fillRect(uint16_t x, uint16_t y, uint16_t sizeX, uint16_t sizeY); // fill a rectangle on display
	void fillRect2(uint16_t x, uint16_t y, uint16_t sizeX, uint16_t sizeY); // fill a rectangle on display (fast)
	void drawChar(uint16_t x, uint16_t y, char value); // draw a character on display
	void drawString(uint16_t x, uint16_t y, char *value, uint8_t length); // draw a string on display 

#endif
