// ====================== [main.c (TerraControl)] ==========================================
/*
*	A small program to control the temperature and lights of a terrarium for an Atmel
*   ATmega32 running @8MHz
*
*	Author: Tobias Braechter
*	Last update: 2020-06-21
*
*/

// ==================================== [includes] =========================================

#include <avr/io.h>
#include <stdint.h>
#include <avr/interrupt.h>

#include "bitOperation.h"
#include "terraControl.h"
#include "display.h"

// ==================================== [pin configuration] ===============================

// PORTA0 (ADC0)		- Heater
// PORTA1 (ADC1)		- Encoder Channel A
// PORTA2 (ADC2)		- Encoder Channel B
// PORTA3 (ADC3)		- Encoder Push Button
// PORTA4 (ADC4)		- Unused
// PORTA5 (ADC5)		- ISP
// PORTA6 (ADC6)		- ISP
// PORTA7 (ADC7)		- ISP

// PORTB0 (XCK/T0)		- Display Data 0
// PORTB1 (T1)			- Display Data 1
// PORTB2 (AIN0/INT2)	- Display Data 2
// PORTB3 (AIN1/OC0)	- Display Data 3
// PORTB4 (SS)			- Display Data 4
// PORTB5 (MOSI)		- Display Data 5
// PORTB6 (MISO)		- Display Data 6
// PORTB7 (SCK)			- Display Data 7

// PORTC0 (SCL)			- Display D/CX (Data or Command)
// PORTC1 (SDA)			- Display RDX (Read Data)
// PORTC2 (TCK)			- Display WRX (Write Data)
// PORTC3 (TMS)			- Unused
// PORTC4 (TDO)			- Unused
// PORTC5 (TDI)			- Unused
// PORTC6 (TOSC1)		- Unused
// PORTC7 (TOSC2)		- Unused

// PORTD0 (RXD)			- LED Channel Red
// PORTD1 (TXD)			- LED Channel Green
// PORTD2 (INT0)		- LED Channel Blue
// PORTD3 (INT1)		- Sensor in
// PORTD4 (OC1B)		- Sensor out
// PORTD5 (OC1A)		- Unused
// PORTD6 (ICP1)		- Unused
// PORTD7 (OC2)			- Unused

#define DDR_HEAT &DDRA,0
#define HEAT &PORTA,0

#define DDR_ENC_A &DDRA,1
#define DDR_ENC_B &DDRA,2
#define DDR_ENC_BTN &DDRA,3
#define ENC_A &PINA,1
#define ENC_B &PINA,2
#define ENC_BTN (PINA&(1<<3))

#define DDR_LED_RED &DDRD,0
#define DDR_LED_GRE &DDRD,1
#define DDR_LED_BLU &DDRD,2
#define LED_RED &PORTD,0
#define LED_GRE &PORTD,1
#define LED_BLU &PORTD,2

#define DDR_SENS_IN &DDRD,3
#define DDR_SENS_OUT &DDRD,4
#define SENS_IN (PIND&(1<<3))
#define SENS_OUT &PORTD,4

// ==================================== [defines] ==========================================

#define NUM_TIMERS 4
#define T_BTN 0
#define T_ACTION 1
#define T_WAIT 2
#define T_CLOCK 3

#define OPTION_PERIOD 100
#define SAVE_PERIOD 60000
#define BTN_PERIOD 10
#define ACTION_PERIOD 10000
#define CLOCK_PERIOD 900
#define SENS_PERIOD 30000

#define DISP_COL_BACK 0,0,0
#define DISP_COL_FRONT 60,60,60
#define DISP_COL_OPT 60,30,30

#define Y_1 30
#define Y_2 70
#define Y_3 100
#define Y_4 130
#define Y_5 160
#define Y_6 190
#define Y_7 220

#define SAVED_PATTERN 170

#define SENS_TIMEOUT 65000
#define SENS_MAX_ERR 1

#define SENS_NOT_READ 0
#define SENS_OK 1

#define LIGHT_OFF 0
#define LIGHT_ON 1
#define LIGHT_DAY 2
#define LIGHT_NIGHT 3

#define NUM_COL 3
#define COL_RED 0
#define COL_GRE 1
#define COL_BLU 2

// ==================================== [variables] ==========================================

volatile uint16_t timer; // counter for main-time
uint16_t timers[NUM_TIMERS]; // counters for different actions
uint16_t clockCycle; // stores the clock cycle for one second
volatile uint8_t pwmCycle; // counter for pwm-cycle
uint8_t seconds; // stores the current seconds

volatile uint8_t optionsChanged; // stores if options have been changed
uint8_t buttonStateOld; // stores the last state of the button
uint8_t encStateOld; // stores the last state of the encoder
uint8_t lastLight; // stores the last state of light
uint8_t sensError; // counts the times the sensor could not be read
uint8_t duty[NUM_COL]; // stores the current duty-cycles for RGB

char buffer[10]; // buffer for drawing strings

// ==================================== [function declaration] ==========================================

void initialize(void); // setting the timers, uart, etc.
void drawInitScreen(void); // draw the main screen
void drawOption(uint8_t index); // draw the given option
void drawData(uint8_t index); // draw the given data
char getNumber(uint8_t value, uint8_t pos, char fill); // get specific number
void loadOptions(void); // load stored options
void loadDefaultOptions(void); // load a default set of options
void saveOptions(void); // store current options
void handleTime(void); // calculate the current time
void handleButton(void); // check if the encoder button was clicked
void handleEncoder(void); // check if the encoder got rotated
void handleLight(void); // determine the current light configuration
uint8_t handleSensor(void); // read sensor data
void handleHeater(void); // control the heater
void handleDisplay(void); // draw the display screen
void resetTimer(uint8_t index); // reset the given timer
uint16_t getTimeDiff(uint8_t index); // get the counter value of the given timer

// ==================================== [program start] ==========================================

int main(void)
{
	initialize();
	resetTimer(T_WAIT);
	while(getTimeDiff(T_WAIT)<100);

	loadOptions();

	DDR_DISP_DATA = ~0;
	dispWrite(DISP_SEL_COM, 0x11);
	resetTimer(T_WAIT);
	while(getTimeDiff(T_WAIT) < 5);
	dispWrite(DISP_SEL_COM, 0x29);
	resetTimer(T_WAIT);
	while(getTimeDiff(T_WAIT)<100);
	drawInitScreen();

	while(1)
	{
		saveOptions();
		handleTime();
		handleButton();
		handleEncoder();
		handleLight();
		uint8_t retVal = handleSensor();
		setBit(&TIMSK, OCIE1A, 1); // enable output compare match interrupt
		if(retVal == SENS_OK)
			sensError = 0;
		else if(retVal != SENS_NOT_READ)
		{
			sensError++;
			if(sensError > SENS_MAX_ERR)
			{
				data[DAT_HYGRO_OK] = 0;
				data[DAT_TEMP_OK] = 0;
			}
		}
		handleHeater();
		handleDisplay();
		if(getTimeDiff(T_ACTION) > ACTION_PERIOD)
			data[DAT_OPTION] = OPT_NONE;
	}
}

void initialize(void)
{
	cli(); // disable global interrupts

	data[DAT_OPTION] = OPT_NONE;

	optionMax[OPT_NONE] = MAX_NONE;
	optionMax[OPT_LIGHT] = MAX_LIGHT;
	optionMax[OPT_DAY_HOUR] = MAX_HOUR;
	optionMax[OPT_DAY_MIN] = MAX_MIN;
	optionMax[OPT_DAY_RED] = MAX_PWM;
	optionMax[OPT_DAY_GRE] = MAX_PWM;
	optionMax[OPT_DAY_BLU] = MAX_PWM;
	optionMax[OPT_DAY_TEMP] = MAX_TEMP;
	optionMax[OPT_NIGHT_HOUR] = MAX_HOUR;
	optionMax[OPT_NIGHT_MIN] = MAX_MIN;
	optionMax[OPT_NIGHT_RED] = MAX_PWM;
	optionMax[OPT_NIGHT_GRE] = MAX_PWM;
	optionMax[OPT_NIGHT_BLU] = MAX_PWM;
	optionMax[OPT_NIGHT_TEMP] = MAX_TEMP;
	optionMax[OPT_HOUR] = MAX_HOUR;
	optionMax[OPT_MIN] = MAX_MIN;
	optionMax[OPT_CLOCK] = MAX_CLOCK;
  
	// LED output configuration
	setBit(DDR_LED_RED, 1); // set pin for red channel as output
	setBit(DDR_LED_GRE, 1); // set pin for green channel as output
	setBit(DDR_LED_BLU, 1); // set pin for blue channel as output
	setBit(LED_RED, 0); // switch pin for red channel off
	setBit(LED_GRE, 0); // switch pin for green channel off
	setBit(LED_BLU, 0); // switch pin for blue channel off

	//sensor configuration
	setBit(DDR_SENS_IN,0); // set sensor in pin as input
	setBit(DDR_SENS_OUT,1); // set sensor out pin as output
	setBit(SENS_OUT,0); // switch sensor out pin off

	// heater output configuration
	setBit(DDR_HEAT, 1); // set heater pin as output
	setBit(HEAT, 0); // switch heater pin off

	// encoder input configuration
	setBit(DDR_ENC_BTN, 0); // set pin for encoder button as input
	setBit(DDR_ENC_A, 0); // set pin for encoder channel a as input
	setBit(DDR_ENC_B, 0); // set pin for encoder channel b as input

	// LCD output configuration
	setBit(DDR_DISP_SEL, 1); // set pin for display selection as output
	setBit(DDR_DISP_READ, 1); // set pin for display read command as output
	setBit(DDR_DISP_WRITE, 1); // set pin for display write command as output
	setBit(DISP_SEL, DISP_SEL_COM); // switch output for display selection to command
	setBit(DISP_READ, 1); // switch output for read comand high
	setBit(DISP_WRITE, 1); // switch output for write command high

	// sensor timer setup
	setBit(&TCCR0, WGM01, 0); // normal mode
	setBit(&TCCR0, WGM00, 0);
	setBit(&TCCR0, CS02, 0); // divider 8 -> period 1us
	setBit(&TCCR0, CS01, 1);
	setBit(&TCCR0, CS00, 0);

	// pwm timer setup
	OCR1A = 100; // pwm interrupt frequency 10 kHz
	setBit(&TCCR1B, WGM13, 0); // ctc mode
	setBit(&TCCR1B, WGM12, 1);
	setBit(&TCCR1A, WGM11, 0);
	setBit(&TCCR1A, WGM10, 0);
	setBit(&TIMSK, OCIE1A, 1); // enable output compare match interrupt
	setBit(&TCCR1B, CS12, 0); // divider 8 -> 1 MHz
	setBit(&TCCR1B, CS11, 1);
	setBit(&TCCR1B, CS10, 0);

	// main timer setup
	OCR2 = 125; // time-base 1 ms
	setBit(&TCCR2, WGM21, 1); // ctc mode
	setBit(&TCCR2, WGM20, 0);
	setBit(&TIMSK, OCIE2, 1); // enable compare match interrupt
	setBit(&TCCR2, CS22, 1);
	setBit(&TCCR2, CS21, 0);
	setBit(&TCCR2, CS20, 0); // divider 64 -> 1/8 MHz

	sei(); // enable global interrupts
}

void drawInitScreen(void)
{
	setColor(DISP_COL_FRONT);
	drawLine(0, 40, 319, 40);
	drawLine(0, 200, 319, 200);
	drawString(110, 130, "TerraControl", 12);
	setColor(DISP_COL_BACK);
	fillRect2(0, 0, 320, 240);

	setColor(DISP_COL_FRONT);
	drawLine(0,40,319,40);
	drawLine(80,40,80,239);
	drawLine(160,40,160,239);
	drawLine(240,40,240,239);

	drawString(10, Y_1, "TerraControl", 12);
	drawString(200, Y_1, "Luftf.", 6);
	drawString(20, Y_3, "Uhr", 3);
	drawString(30, Y_4, "R", 1);
	drawString(30, Y_5, "G", 1);
	drawString(30, Y_6, "B", 1);
	drawString(10, Y_7, "Temp", 4);
	drawString(90,Y_2,"Tag",3);
	drawChar(116,Y_3,':');
	drawString(170,Y_2,"Nacht",5);
	drawChar(196,Y_3,':');
	drawChar(276,Y_3,':');
	drawChar(250,Y_4,'T');
	drawString(250, Y_5, "Licht", 5);

	for(uint8_t i = 0; i < NUM_OPT; i++)
		drawOption(i);

	for(uint8_t i = 0; i < NUM_DAT; i++)
		drawData(i);
}

void drawOption(uint8_t index)
{
	switch(index)
	{
		case OPT_LIGHT:
			if(options[index] == OPT_LIGHT_AUTO)
				drawString(250,Y_6,"Auto",4);
			else if(options[index] == OPT_LIGHT_ON)
				drawString(250,Y_6,"On",2);
			else
				drawString(250,Y_6,"Off",3);
			break;
		case OPT_DAY_HOUR:
			buffer[0] = getNumber(options[OPT_DAY_HOUR],1,'0');
			buffer[1] = getNumber(options[OPT_DAY_HOUR],0,'0');
			drawString(90,Y_3,buffer,2);
			break;
		case OPT_DAY_MIN:
			buffer[0] = getNumber(options[OPT_DAY_MIN],1,'0');
			buffer[1] = getNumber(options[OPT_DAY_MIN],0,'0');
			drawString(120,Y_3,buffer,2);
			break;
		case OPT_DAY_RED:
			buffer[0] = getNumber(options[index],2,' ');
			buffer[1] = getNumber(options[index],1,' ');
			buffer[2] = getNumber(options[index],0,'0');
			buffer[3] = '%';
			drawString(100,Y_4,buffer,4);
			break;
		case OPT_DAY_GRE:
			buffer[0] = getNumber(options[index],2,' ');
			buffer[1] = getNumber(options[index],1,' ');
			buffer[2] = getNumber(options[index],0,'0');
			buffer[3] = '%';
			drawString(100,Y_5,buffer,4);
			break;
		case OPT_DAY_BLU:
			buffer[0] = getNumber(options[index],2,' ');
			buffer[1] = getNumber(options[index],1,' ');
			buffer[2] = getNumber(options[index],0,'0');
			buffer[3] = '%';
			drawString(100,Y_6,buffer,4);
			break;
		case OPT_DAY_TEMP:
			buffer[0] = getNumber(options[index],2,' ');
			buffer[1] = getNumber(options[index],1,' ');
			buffer[2] = getNumber(options[index],0,'0');
			buffer[3] = '°';
			buffer[4] = 'C';
			drawString(95,Y_7,buffer,5);
			break;
		case OPT_NIGHT_HOUR:
			buffer[0] = getNumber(options[OPT_NIGHT_HOUR],1,'0');
			buffer[1] = getNumber(options[OPT_NIGHT_HOUR],0,'0');
			drawString(170,Y_3,buffer,2);
			break;
		case OPT_NIGHT_MIN:
			buffer[0] = getNumber(options[OPT_NIGHT_MIN],1,'0');
			buffer[1] = getNumber(options[OPT_NIGHT_MIN],0,'0');
			drawString(200,Y_3,buffer,2);
			break;
		case OPT_NIGHT_RED:
			buffer[0] = getNumber(options[index],2,' ');
			buffer[1] = getNumber(options[index],1,' ');
			buffer[2] = getNumber(options[index],0,'0');
			buffer[3] = '%';
			drawString(180,Y_4,buffer,4);
			break;
		case OPT_NIGHT_GRE:
			buffer[0] = getNumber(options[index],2,' ');
			buffer[1] = getNumber(options[index],1,' ');
			buffer[2] = getNumber(options[index],0,'0');
			buffer[3] = '%';
			drawString(180,Y_5,buffer,4);
			break;
		case OPT_NIGHT_BLU:
			buffer[0] = getNumber(options[index],2,' ');
			buffer[1] = getNumber(options[index],1,' ');
			buffer[2] = getNumber(options[index],0,'0');
			buffer[3] = '%';
			drawString(180,Y_6,buffer,4);
			break;
		case OPT_NIGHT_TEMP:
			buffer[0] = getNumber(options[index],2,' ');
			buffer[1] = getNumber(options[index],1,' ');
			buffer[2] = getNumber(options[index],0,'0');
			buffer[3] = '°';
			buffer[4] = 'C';
			drawString(175,Y_7,buffer,5);
			break;
		case OPT_HOUR:
			buffer[0] = getNumber(options[OPT_HOUR],1,'0');
			buffer[1] = getNumber(options[OPT_HOUR],0,'0');
			drawString(250,Y_3,buffer,2);
			break;
		case OPT_MIN:
			buffer[0] = getNumber(options[OPT_MIN],1,'0');
			buffer[1] = getNumber(options[OPT_MIN],0,'0');
			drawString(280,Y_3,buffer,2);
			break;
		case OPT_CLOCK:
			buffer[0] = getNumber(options[index],2,' ');
			buffer[1] = getNumber(options[index],1,' ');
			buffer[2] = getNumber(options[index],0,'0');
			drawString(260,Y_4,buffer,3);
			break;
	}
}

void drawData(uint8_t index)
{
	switch(index)
	{
		case DAT_TEMP:
		case DAT_TEMP_OK:
			if(data[DAT_TEMP_OK])
			{
				buffer[0] = getNumber(data[DAT_TEMP],2,' ');
				buffer[1] = getNumber(data[DAT_TEMP],1,' ');
				buffer[2] = getNumber(data[DAT_TEMP],0,'0');
				buffer[3] = '°';
				buffer[4] = 'C';
				drawString(255,Y_7,buffer,5);
			}
			else
			{
				drawString(255,Y_7," --°C",5);
			}
			break;
		case DAT_HYGRO:
		case DAT_HYGRO_OK:
			if(data[DAT_HYGRO_OK])
			{
				buffer[0] = getNumber(data[DAT_HYGRO],2,' ');
				buffer[1] = getNumber(data[DAT_HYGRO],1,' ');
				buffer[2] = getNumber(data[DAT_HYGRO],0,'0');
				buffer[3] = '%';
				drawString(260,Y_1,buffer,4);
			}
			else
			{
				drawString(260,Y_1," --%",4);
			}
			break;
		case DAT_DAYTIME:
			if(data[DAT_DAYTIME] == DAYTIME_DAY)
				drawString(250,Y_2,"Tag",3);
			else
				drawString(250,Y_2,"Nacht",5);
			break;
	}
}

char getNumber(uint8_t value, uint8_t pos, char fill)
{
	for(uint8_t i=0; i < pos; i++)
		value /= 10;
	if(!value)
		return fill;
	return (value%10+'0');
}

void loadOptions(void)
{
	cli(); // while reading from EEPROM, disable interrupts
	uint8_t checkByte;
	while(readBit(&EECR, EEWE)); // wait for possible writing to finish
	EEAR = 0;
	setBit(&EECR, EERE, 1); // enable read operation
	checkByte = EEDR; // load target data to cache
	setBit(&EECR, EERE, 0); // disable read operation
	
	if(checkByte == SAVED_PATTERN)
	{
		for(uint8_t i=0; i < NUM_OPT; i++)
		{
			while(readBit(&EECR, EEWE)); // wait for possible writing to finish
			EEAR = i+1; // write target address
			setBit(&EECR, EERE, 1); // enable read operation
			options[i] = EEDR; // load target data to cache
			setBit(&EECR, EERE, 0); // disable read operation
		}
	}
	else
	{
		loadDefaultOptions();
	}
	EECR = 0; // clear any operation bits
	for(uint8_t i = 0; i < NUM_OPT; i++)
		optionsCache[i] = options[i];
	sei(); // re-enable interrupts
}

void loadDefaultOptions(void)
{
	options[OPT_NONE] = MAX_NONE;
	options[OPT_LIGHT] = OPT_LIGHT_AUTO;
	options[OPT_DAY_HOUR] = 8;
	options[OPT_DAY_MIN] = 0;
	options[OPT_DAY_RED] = 25;
	options[OPT_DAY_GRE] = 25;
	options[OPT_DAY_BLU] = 0;
	options[OPT_DAY_TEMP] = 0;
	options[OPT_NIGHT_HOUR] = 22;
	options[OPT_NIGHT_MIN] = 0;
	options[OPT_NIGHT_RED] = 15;
	options[OPT_NIGHT_GRE] = 7;
	options[OPT_NIGHT_BLU] = 0;
	options[OPT_NIGHT_TEMP] = 0;
	options[OPT_HOUR] = 0;
	options[OPT_MIN] = 0;
	options[OPT_CLOCK] = 109;
}

void saveOptions(void)
{
	if(optionsChanged && getTimeDiff(T_ACTION) > SAVE_PERIOD)
	{
		cli(); // while writing, disable interrupts
		optionsChanged = 0;
		while(readBit(&EECR, EEWE)); // wait for possible writing to finish
		EEAR = 0; // write target address
		EEDR = SAVED_PATTERN; // write target data
		EECR = (1 << EEMWE);
		EECR |= (1 << EEWE);

		for(uint8_t i=0; i < NUM_OPT; i++)
		{
			while(readBit(&EECR, EEWE)); // wait for possible writing to finish
			EEAR = i+1; // write target address
			EEDR = options[i]; // write target data
			EECR = (1 << EEMWE);
			// the write enable bit has to be set within 4 cycles after master write enable
			// setBit(...) is too slow!
			EECR |= (1 << EEWE);
		}
		while(readBit(&EECR, EEWE)); // wait for possible writing to finish
		EECR = 0; // clear any operation bits
		sei(); // re-enable interrupts
	}
}

void handleTime(void)
{
	clockCycle = CLOCK_PERIOD + options[OPT_CLOCK];
	if(getTimeDiff(T_CLOCK) >= clockCycle)
	{
		resetTimer(T_CLOCK);
		seconds++;
		if(seconds > MAX_SEC)
		{
			seconds -= MAX_SEC;
			options[OPT_MIN]++;
		}
		if(options[OPT_MIN] > MAX_MIN)
		{
			options[OPT_MIN] = 0;
			options[OPT_HOUR]++;
		}
		if(options[OPT_HOUR] > MAX_HOUR)
		{
			options[OPT_HOUR] = 0;
		}
		uint16_t minutesDay = options[OPT_DAY_HOUR] * 60 + options[OPT_DAY_MIN];
		uint16_t minutesNight = options[OPT_NIGHT_HOUR] * 60 + options[OPT_NIGHT_MIN];
		uint16_t minutesCurrent = options[OPT_HOUR] * 60 + options[OPT_MIN];
		if(minutesDay < minutesNight)
		{
			if(minutesDay <= minutesCurrent && minutesCurrent < minutesNight)
				data[DAT_DAYTIME] = DAYTIME_DAY;
			else
				data[DAT_DAYTIME] = DAYTIME_NIGHT;
		}
		else
		{
			if(minutesNight <= minutesCurrent && minutesCurrent < minutesDay)
				data[DAT_DAYTIME] = DAYTIME_NIGHT;
			else
				data[DAT_DAYTIME] = DAYTIME_DAY;
		}
	}
}

void handleButton(void)
{
	if(!buttonStateOld && ENC_BTN && getTimeDiff(T_BTN) > BTN_PERIOD)
	{
		resetTimer(T_BTN);
		resetTimer(T_ACTION);
		data[DAT_OPTION]++;
		if(data[DAT_OPTION] == NUM_OPT)
			data[DAT_OPTION] = 0;
	}
	buttonStateOld = ENC_BTN;
}

void handleEncoder(void)
{
	uint8_t encState = readBit(ENC_A) + (readBit(ENC_B) << 1);
	if(encState == 0 && encStateOld == 1 && data[DAT_OPTION] != OPT_NONE)
	{
		resetTimer(T_ACTION);
		optionsChanged = 1;
		if(options[data[DAT_OPTION]] < optionMax[data[DAT_OPTION]])
			options[data[DAT_OPTION]]++;
		else
			options[data[DAT_OPTION]] = 0;
	}
	else if(encState == 2 && encStateOld == 3 && data[DAT_OPTION] != OPT_NONE)
	{
		resetTimer(T_ACTION);
		optionsChanged = 1;
		if(options[data[DAT_OPTION]] > 0)
			options[data[DAT_OPTION]]--;
		else
			options[data[DAT_OPTION]] = optionMax[data[DAT_OPTION]];
	}
	encStateOld = encState;
}

void handleLight(void)
{
	uint8_t light = LIGHT_OFF;
	uint8_t lightOption = 0;
	if(data[DAT_OPTION] >= OPT_DAY_RED && data[DAT_OPTION] <= OPT_DAY_BLU)
	{
		light = LIGHT_DAY;
		lightOption = 1;
	}
	else if(data[DAT_OPTION] >= OPT_NIGHT_RED && data[DAT_OPTION] <= OPT_NIGHT_BLU)
	{
		light = LIGHT_NIGHT;
		lightOption = 1;
	}
	else if(options[OPT_LIGHT] == OPT_LIGHT_ON)
		light = LIGHT_ON;
	else if(options[OPT_LIGHT] == OPT_LIGHT_AUTO)
	{
		if(data[DAT_DAYTIME] == DAYTIME_DAY)
			light = LIGHT_DAY;
		else
			light = LIGHT_NIGHT;
	}

	if(lightOption || light != lastLight)
	{
		lastLight = light;
		if(light == LIGHT_OFF)
		{
			duty[COL_RED] = 0;
			duty[COL_GRE] = 0;
			duty[COL_BLU] = 0;
		}
		else if(light == LIGHT_ON)
		{
			duty[COL_RED] = MAX_PWM;
			duty[COL_GRE] = MAX_PWM;
			duty[COL_BLU] = MAX_PWM;
		}
		else if(light == LIGHT_DAY)
		{
			duty[COL_RED] = options[OPT_DAY_RED];
			duty[COL_GRE] = options[OPT_DAY_GRE];
			duty[COL_BLU] = options[OPT_DAY_BLU];
		}
		else if(light == LIGHT_NIGHT)
		{
			duty[COL_RED] = options[OPT_NIGHT_RED];
			duty[COL_GRE] = options[OPT_NIGHT_GRE];
			duty[COL_BLU] = options[OPT_NIGHT_BLU];
		}
	}
}

uint8_t handleSensor(void)
{
	if(getTimeDiff(T_WAIT) > SENS_PERIOD)
	{
		uint8_t inputBits[40];
		for(uint8_t i = 0; i < 40; i++)
			inputBits[i] = 0;
		uint8_t inputData[5];
		uint8_t index = 0;
		uint16_t timeout = 0;
		
		resetTimer(T_WAIT);
		setBit(SENS_OUT,1);
		while(getTimeDiff(T_WAIT) < 18);
		setBit(&TIMSK, OCIE1A, 0); // disable pwm output compare match interrupt
		setBit(SENS_OUT,0);

		TCNT0 = 0;
		while(TCNT0 < 30)
		{
			if(++timeout == SENS_TIMEOUT)
				return 2;
			if(SENS_IN)
				TCNT0 = 0;
		}
		while(!SENS_IN)
			if(++timeout == SENS_TIMEOUT)
				return 3;
		while(SENS_IN)
			if(++timeout == SENS_TIMEOUT)
				return 4;
		timeout = 0;

		while(index < 40)
		{
			while(!SENS_IN)
				if(++timeout == SENS_TIMEOUT)
					return 5;
			TCNT0 = 0;
			while(SENS_IN)
				if(++timeout == SENS_TIMEOUT)
					return 6;

			if(TCNT0 > 50)
				inputBits[index] = 1;
			index++;
		}

		setBit(&TIMSK, OCIE1A, 1); // enable pwm output compare match interrupt

		for(uint8_t i = 0; i < 40; i++)
		{
			index = i / 8;
			uint8_t bit = 7 - (i%8);
			setBit(inputData+index,bit,inputBits[i]);
		}

		uint8_t sum = inputData[0] + inputData[1] + inputData[2] + inputData[3];
		if(sum != inputData[4])
		{
			return 7;
		}
		else
		{
			data[DAT_HYGRO_OK] = 1;
			uint16_t hygro = (inputData[0] << 8) + inputData[1];
			data[DAT_HYGRO] = hygro / 10;
			if(hygro % 10 >= 5)
				data[DAT_HYGRO]++;

			if(readBit(inputData+2,7))
				data[DAT_TEMP_OK] = 0;
			else
			{
				data[DAT_TEMP_OK] = 1;
				uint16_t temp = (inputData[2] << 8) + inputData[3];
				data[DAT_TEMP] = temp / 10;
				if(temp % 10 >= 5)
					data[DAT_TEMP]++;
			}
			return SENS_OK;
		}
	}
	return SENS_NOT_READ;
}

void handleHeater(void)
{
	uint8_t temp = options[OPT_DAY_TEMP];
	if(data[DAT_DAYTIME] == DAYTIME_NIGHT)
		temp = options[OPT_NIGHT_TEMP];

	if(!data[DAT_TEMP_OK] || data[DAT_TEMP] > temp)
		setBit(HEAT, 0);
	else
		setBit(HEAT, 1);
}

void handleDisplay(void)
{
	for(uint8_t i = 0; i < NUM_OPT; i++)
	{
		if(options[i] != optionsCache[i])
		{
			uint8_t cache = options[i];
			options[i] = optionsCache[i];
			setColor(DISP_COL_BACK);
			drawOption(i);
			if(i == data[DAT_OPTION])
				setColor(DISP_COL_OPT);
			else
				setColor(DISP_COL_FRONT);
			options[i] = cache;
			optionsCache[i] = cache;
			drawOption(i);
		}
	}
	for(uint8_t i = 0; i < NUM_DAT; i++)
	{
		if(data[i] != dataCache[i])
		{
			if(i == DAT_OPTION)
			{
				setColor(DISP_COL_FRONT);
				drawOption(dataCache[i]);
				setColor(DISP_COL_OPT);
				drawOption(data[i]);
				dataCache[i] = data[i];
			}
			else
			{
				uint8_t cache = data[i];
				data[i] = dataCache[i];
				setColor(DISP_COL_BACK);
				drawData(i);
				setColor(DISP_COL_FRONT);
				data[i] = cache;
				dataCache[i] = cache;
				drawData(i);
			}
		}
	}
}

void resetTimer(uint8_t index)
{
	if(index < NUM_TIMERS)
	{
		cli();
		timers[index] = timer;
		sei();
	}
}

uint16_t getTimeDiff(uint8_t index)
{
	if(index < NUM_TIMERS)
	{
		cli();
		uint16_t result = timer - timers[index];
		sei();
		return result;
	}
	return 0;
}

ISR(TIMER1_COMPA_vect) // PWM
{
	pwmCycle++;
	if(pwmCycle > MAX_PWM)
	{
		setBit(LED_RED,duty[COL_RED]);
		setBit(LED_GRE,duty[COL_GRE]);
		setBit(LED_BLU,duty[COL_BLU]);
		pwmCycle = 0;
	}
	if(pwmCycle == duty[COL_RED])
		setBit(LED_RED,0);
	if(pwmCycle == duty[COL_GRE])
		setBit(LED_GRE,0);
	if(pwmCycle == duty[COL_BLU])
		setBit(LED_BLU,0);
}

ISR(TIMER2_COMP_vect) // internal clock
{
	timer++;
}
