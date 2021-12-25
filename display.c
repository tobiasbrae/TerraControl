// ==================================== [display.c] =============================
/*
*	This library provides functions to control a 2.8in display on an Atmel ATmega.
*
*	Author: Tobias Braechter
*	Last update: 2020-06-21
*
*/

#include "display.h"

void dispWrite(uint8_t sel, uint8_t data)
{
	DISP_DATA_OUT = data;
	setBit(DISP_SEL, sel);
	setBit(DISP_WRITE, 0);
	setBit(DISP_WRITE, 1);
}

void setColor(uint8_t red, uint8_t green, uint8_t blue)
{
	dispColor[0] = blue << 2;
	dispColor[1] = green << 2;
	dispColor[2] = red << 2;
}

void storePosition(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	if(x1 < x2)
	{
		xStart = x1;
		xEnd = x2;
		yStart = y1;
		yEnd = y2;
	}
	else
	{
		xStart = x2;
		xEnd = x1;
		yStart = y2;
		yEnd = y1;
	}
}

void drawPoint(uint16_t x, uint16_t y)
{
	if(x <= DISP_MAX_X && y <= DISP_MAX_Y)
	{
		y = DISP_MAX_Y - y;
		x = DISP_MAX_X - x;

		dispWrite(DISP_SEL_COM, 0x2A);
		dispWrite(DISP_SEL_DAT, y >> 8);
		dispWrite(DISP_SEL_DAT, y);
		dispWrite(DISP_SEL_DAT, y >> 8);
		dispWrite(DISP_SEL_DAT, y);

		dispWrite(DISP_SEL_COM, 0x2B);
		dispWrite(DISP_SEL_DAT, x >> 8);
		dispWrite(DISP_SEL_DAT, x);
		dispWrite(DISP_SEL_DAT, x >> 8);
		dispWrite(DISP_SEL_DAT, x);

		dispWrite(DISP_SEL_COM, 0x2C);
		dispWrite(DISP_SEL_DAT, dispColor[0]);
		dispWrite(DISP_SEL_DAT, dispColor[1]);
		dispWrite(DISP_SEL_DAT, dispColor[2]);
	}
}

void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	storePosition(x1, y1, x2, y2);
	if(x1 == x2)
	{
		if(y2 > y1)
			for(uint16_t y = y1; y <= y2; y++)
				drawPoint(x1, y);
		else
			for(uint16_t y = y2; y <= y1; y++)
				drawPoint(x1, y);
	}
	else if(y1 == y2)
	{
		for(uint16_t x = xStart; x <= xEnd; x++)
			drawPoint(x,y1);
	}
	else
	{
		double m = ((double)yEnd - (double)yStart)/((double)xEnd-(double)xStart);
		for(uint16_t x = xStart; x <= xEnd; x++)
		{
			double y = (double) yStart;
			y += m * (double) (x - xStart);
			drawPoint(x, (uint16_t)y);
		}
	}
}

void drawRect(uint16_t x, uint16_t y, uint16_t sizeX, uint16_t sizeY)
{
	drawLine(x, y, x+sizeX-1, y);
	drawLine(x+sizeX-1, y, x+sizeX-1, y+sizeY-1);
	drawLine(x+sizeX-1, y+sizeY-1, x, y+sizeY-1);
	drawLine(x, y+sizeY-1, x, y);
}

void fillRect(uint16_t x, uint16_t y, uint16_t sizeX, uint16_t sizeY)
{
	for(uint16_t curX = x; curX < (x+sizeX); curX++)
		for(uint16_t curY = y; curY < (y+sizeY); curY++)
			drawPoint(curX, curY);
}

void fillRect2(uint16_t x, uint16_t y, uint16_t sizeX, uint16_t sizeY)
{
	if(x <= DISP_MAX_X && y <= DISP_MAX_Y)
	{
		uint16_t x2 = x + sizeX;
		if(x2 > DISP_MAX_X)
			x2 = DISP_MAX_X;

		uint16_t y2 = y + sizeY;
		if(y2 > DISP_MAX_Y)
			y2 = DISP_MAX_Y;

		x = DISP_MAX_X - x;
		y = DISP_MAX_Y - y;
		x2 = DISP_MAX_X - x2;
		y2 = DISP_MAX_Y - y2;

		dispWrite(DISP_SEL_COM, 0x2A);
		dispWrite(DISP_SEL_DAT, y2 >> 8);
		dispWrite(DISP_SEL_DAT, y2);
		dispWrite(DISP_SEL_DAT, y >> 8);
		dispWrite(DISP_SEL_DAT, y);

		dispWrite(DISP_SEL_COM, 0x2B);
		dispWrite(DISP_SEL_DAT, x2 >> 8);
		dispWrite(DISP_SEL_DAT, x2);
		dispWrite(DISP_SEL_DAT, x >> 8);
		dispWrite(DISP_SEL_DAT, x);

		dispWrite(DISP_SEL_COM, 0x2C);
		dispWrite(DISP_SEL_DAT, dispColor[0]);
		dispWrite(DISP_SEL_DAT, dispColor[1]);
		dispWrite(DISP_SEL_DAT, dispColor[2]);

		for(uint16_t curX = 0; curX < sizeX; curX++)
		{
			for(uint16_t curY = 0; curY < sizeY; curY++)
			{
				dispWrite(DISP_SEL_COM, 0x3C);
				dispWrite(DISP_SEL_DAT, dispColor[0]);
				dispWrite(DISP_SEL_DAT, dispColor[1]);
				dispWrite(DISP_SEL_DAT, dispColor[2]);
			}
		}
	}
}

void drawChar(uint16_t x, uint16_t y, char value)
{
	switch(value)
	{
		case '°':
			drawRect(x,y-14,3,3);
			charX += 5;
			break;
		case ' ':
			charX += 11;
			break;
		case '!':
			drawLine(x,y-4,x,y-14);
			drawLine(x,y,x,y-1);
			charX += 3;
			break;
		case '%':
			drawRect(x,y-14,3,3);
			drawRect(x+8,y-2,3,3);
			drawLine(x,y,x+10,y-14);
			charX += 13;
			break;
		case '-':
			drawLine(x+1,y-7,x+9,y-7);
			charX += 13;
			break;
		case '.':
			fillRect(x,y-1,2,2);
			charX += 4;
			break;
		case '0':
			drawRect(x,y-14,11,15);
			charX += 13;
			break;
		case '1':
			drawLine(x+10,y-14,x+10,y);
			charX += 13;
			break;
		case '2':
			drawLine(x,y-14,x+10,y-14);
			drawLine(x+10,y-14,x+10,y-7);
			drawLine(x,y-7,x+10,y-7);
			drawLine(x,y-7,x,y);
			drawLine(x,y,x+10,y);
			charX += 13;
			break;
		case '3':
			drawLine(x+10,y,x+10,y-14);
			drawLine(x,y-14,x+10,y-14);
			drawLine(x,y-7,x+10,y-7);
			drawLine(x,y,x+10,y);
			charX += 13;
			break;
		case '4':
			drawLine(x,y-14,x,y-7);
			drawLine(x,y-7,x+10,y-7);
			drawLine(x+10,y-14,x+10,y);
			charX += 13;
			break;
		case '5':
			drawLine(x+10,y-14,x,y-14);
			drawLine(x,y-14,x,y-7);
			drawLine(x,y-7,x+10,y-7);
			drawLine(x+10,y-7,x+10,y);
			drawLine(x+10,y,x,y);
			charX += 13;
			break;
		case '6':
			drawRect(x,y-7,11,8);
			drawLine(x,y-7,x,y-14);
			drawLine(x,y-14,x+10,y-14);
			charX += 13;
			break;
		case '7':
			drawLine(x,y-7,x,y-14);
			drawLine(x,y-14,x+10,y-14);
			drawLine(x+10,y-14,x+10,y);
			charX += 13;
			break;
		case '8':
			drawRect(x,y-14,11,8);
			drawRect(x,y-7,11,8);
			charX += 13;
			break;
		case '9':
			drawRect(x,y-14,11,8);
			drawLine(x+10,y-7,x+10,y);
			drawLine(x+10,y,x,y);
			charX += 13;
			break;
		case ':':
			fillRect(x,y-5,2,2);
			fillRect(x,y-10,2,2);
			charX += 4;
			break;
		case 'A':
			drawLine(x,y,x,y-12);
			drawLine(x+10,y,x+10,y-12);
			drawLine(x,y-7,x+10,y-7);
			drawLine(x+2,y-14,x+8,y-14);
			drawPoint(x+1,y-13);
			drawPoint(x+9,y-13);
			charX += 13;
			break;
		case 'B':
			drawLine(x,y,x,y-14);
			drawLine(x,y,x+9,y);
			drawLine(x,y-7,x+9,y-7);
			drawLine(x,y-14,x+9,y-14);
			drawLine(x+10,y-2,x+10,y-5);
			drawLine(x+10,y-9,x+10,y-12);
			drawPoint(x+9,y-1);
			drawPoint(x+9,y-6);
			drawPoint(x+9,y-8);
			drawPoint(x+9,y-13);
			charX += 13;
			break;
		case 'C':
			drawLine(x,y,x+10,y);
			drawLine(x,y-14,x+10,y-14);
			drawLine(x,y,x,y-14);
			charX += 13;
			break;
		case 'D':
			break;
		case 'E':
			break;
		case 'F':
			break;
		case 'G':
			drawLine(x,y,x,y-14);
			drawLine(x+1,y-14,x+10,y-14);
			drawLine(x+10,y-13,x+10,y-12);
			drawLine(x+1,y,x+10,y);
			drawLine(x+10,y-1,x+10,y-7);
			drawLine(x+9,y-7,x+4,y-7);
			drawPoint(x+9,y-6);
			charX += 13;
			break;
		case 'H':
			drawLine(x,y,x,y-14);
			drawLine(x, y-7,x+10,y-7);
			drawLine(x+10,y,x+10,y-14);
			charX += 13;
			break;
		case 'I':
			break;
		case 'J':
			break;
		case 'K':
			break;
		case 'L':
			drawLine(x,y,x+10,y);
			drawLine(x,y,x,y-14);
			charX += 13;
			break;
		case 'M':
			break;
		case 'N':
			drawLine(x,y,x,y-14);
			drawLine(x, y-14,x+10,y);
			drawLine(x+10,y,x+10,y-14);
			charX += 13;
			break;
		case 'O':
			drawRect(x,y-14,11,15);
			charX += 13;
			break;
		case 'P':
			break;
		case 'Q':
			break;
		case 'R':
			drawRect(x,y-14,11,7);
			drawLine(x,y-8,x,y);
			drawPoint(x+1,y-8);
			drawLine(x+2,y-8,x+10,y);
			charX += 13;
			break;
		case 'S':
			break;
		case 'T':
			drawLine(x,y-14,x+10,y-14);
			drawLine(x+5,y,x+5,y-14);
			charX += 13;
			break;
		case 'U':
			drawLine(x,y-14,x,y);
			drawLine(x+10,y-14,x+10,y);
			drawLine(x,y,x+10,y);
			charX += 13;
			break;
		case 'V':
			break;
		case 'W':
			break;
		case 'X':
			break;
		case 'Y':
			break;
		case 'Z':
			break;
		case 'a':
			drawRect(x,y-4,9,5);
			drawLine(x+8,y-4,x+8,y-8);
			drawLine(x,y-8,x+8,y-8);
			drawPoint(x,y-7);
			charX += 11;
			break;
		case 'b':
			break;
		case 'c':
			drawLine(x,y,x+8,y);
			drawLine(x,y-8,x+8,y-8);
			drawLine(x,y,x,y-8);
			charX += 11;
			break;
		case 'd':
			drawRect(x,y-8,9,9);
			drawLine(x+8,y-12,x+8,y);
			charX += 11;
			break;
		case 'e':
			drawLine(x,y,x+8,y);
			drawLine(x,y-4,x+8,y-4);
			drawLine(x,y-8,x+8,y-8);
			drawLine(x,y,x,y-8);
			drawLine(x+8,y-4,x+8,y-8);
			charX += 11;
			break;
		case 'f':
			drawLine(x+2,y,x+2,y-13);
			drawLine(x,y-8,x+4,y-8);
			drawLine(x+2,y-13,x+6,y-13);
			charX += 9;
			break;
		case 'g':
			drawLine(x,y,x+7,y);
			drawLine(x,y,x,y-8);
			drawLine(x,y-8,x+7,y-8);
			drawLine(x+7,y-9,x+7,y+4);
			drawLine(x,y+4,x+7,y+4);
			charX += 9;
			break;
		case 'h':
			drawLine(x,y,x,y-14);
			drawLine(x,y-8,x+6,y-8);
			drawLine(x+6,y-8,x+6,y);
			charX += 9;
			break;
		case 'i':
			drawLine(x,y,x,y-8);
			drawPoint(x,y-10);
			charX += 3;
			break;
		case 'j':
			break;
		case 'k':
			break;
		case 'l':
			drawLine(x,y,x,y-14);
			charX += 3;
			break;
		case 'm':
			drawLine(x,y-9,x,y);
			drawLine(x+4,y-8,x+4,y);
			drawLine(x+8,y-8,x+8,y);
			drawLine(x,y-8,x+8,y-8);
			charX += 11;
			break;
		case 'n':
			drawLine(x,y,x,y-9);
			drawLine(x,y-8,x+7,y-8);
			drawLine(x+7,y-8,x+7,y);
			charX += 10;
			break;
		case 'o':
			drawRect(x,y-8,9,9);
			charX += 11;
			break;
		case 'p':
			drawLine(x,y-9,x,y+4);
			drawLine(x,y-8,x+7,y-8);
			drawLine(x+7,y-8,x+7,y);
			drawLine(x+7,y,x,y);
			charX += 9;
			break;
		case 'q':
			break;
		case 'r':
			drawLine(x,y,x,y-9);
			drawLine(x,y-8,x+7,y-8);
			drawLine(x+7,y-8,x+7,y-6);
			charX += 10;
			break;
		case 's':
			break;
		case 't':
			drawLine(x+2,y,x+2,y-10);
			drawLine(x,y-8,x+4,y-8);
			drawLine(x+2,y,x+6,y);
			charX += 9;
			break;
		case 'u':
			drawLine(x,y,x,y-8);
			drawLine(x,y,x+8,y);
			drawLine(x+8,y-8,x+8,y);
			charX += 11;
			break;
		case 'v':
			break;
		case 'w':
			drawLine(x,y-8,x,y);
			drawLine(x+4,y-7,x+4,y);
			drawLine(x+8,y-8,x+8,y);
			drawLine(x,y,x+8,y);
			charX += 11;
			break;
		case 'x':
			break;
		case 'y':
			break;
		case 'z':
			break;
	}
}

void drawString(uint16_t x, uint16_t y, char *value, uint8_t length)
{
	charX = x;
	for(uint8_t i = 0; i < length; i++)
		drawChar(charX, y, value[i]);
}
