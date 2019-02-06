#include <Arduino.h>
uint8_t moisture_pin[3] = {A5, A6, A7}; // moisture sensor pin
uint8_t solenoid_pin[3] = {5, 6, 7};	// solenoid pin

#define humidAir_pin A10 // yellow
#define TempAir_pin A12  // white

uint32_t currentTime = millis(); // timer value

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <amt1001_ino.h>

// include the SD library:
#include <SPI.h>
#include <SD.h>

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
const int chipSelect = 53;

File dataFile;

LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display

bool valveOn[3] = {false, false, false}; // save state of valve

// set range of moisture limit
uint8_t _min[3] = {45, 45, 45};
uint8_t _max[3] = {55, 55, 55};

uint16_t moisture[3]; // Declare variable to keep measured value

void readAir();
void moistureControl();
void init_SD();

void setup()
{
	Serial.begin(9600);
	lcd.init();
	lcd.backlight();
	for (int i = 0; i < 3; i++)
	{
		pinMode(moisture_pin[i], INPUT);  // Declare pinmode of sensor
		pinMode(solenoid_pin[i], OUTPUT); // Declare pinmode of solenoid
	}
	pinMode(TempAir_pin, INPUT);
	pinMode(humidAir_pin, INPUT);

	init_SD();

	delay(1000); // delay to prepare to run
}

void loop()
{
	// set timer of work (millisecond)
	if (millis() - currentTime > 500)
	{
		// readAir();
		// Serial.println();
		currentTime = millis(); // reset timer
	}
}

void readAir()
{
	// Get Temperature
	uint16_t temperature = analogRead(TempAir_pin);
	// temperature = amt1001_gettemperature(temperature);
	Serial.print(temperature);
	Serial.print(" : ");
	// Get Humidity
	uint16_t humid = analogRead(humidAir_pin);
	double volt = (double)humid * (5.0 / 1023.0);
	humid = amt1001_gethumidity(volt);

	Serial.println(humid);
}

void moistureControl()
{
	/*
    When we took the readings from the dry soil, 
    then the sensor value was 550 and in the wet soil,
    the sensor value was 10
    */
	for (int i = 0; i < 2; i++)
	{
		moisture[i] = analogRead(moisture_pin[i]); // read sensor
		// moisture[i] = constrain(moisture[i], 0, 550);   // 550 - 10
		// moisture[i] = map(moisture[i], 550, 0, 0, 100); // map value to percentage
		// Serial.print(" : ");
		// Serial.print(moisture[i]);

		// lcd.clear();
		// lcd.setCursor(0, i);
		// lcd.print("Humid :");
		// lcd.setCursor(9, i);
		// lcd.print(moisture[i]);
		// solenoid controlh
		if (_min[i] > moisture[i])
		{
			// digitalWrite(solenoid_pin[i], 1);
			valveOn[i] = true;
		}
		else if (valveOn[i] && moisture[i] > _max[i])
		{
			// digitalWrite(solenoid_pin[i], 0);
			valveOn[i] = false;
		}
	}
}

void init_SD()
{
	Serial.print("Initializing SD card...");
	// make sure that the default chip select pin is set to
	// output, even if you don't use it:
	pinMode(SS, OUTPUT);

	// see if the card is present and can be initialized:
	if (!SD.begin(chipSelect))
	{
		Serial.println("Card failed, or not present");
		// don't do anything more:
		while (1)
			;
	}
	Serial.println("card initialized.");

	// Open up the file we're going to log to!
	dataFile = SD.open("datalog.txt", FILE_WRITE);
	if (!dataFile)
	{
		Serial.println("error opening datalog.txt");
		// Wait forever since we cant write data
		while (1)
			;
	}
}

void readSD()
{
	String dataString = "";

	// read three sensors and append to the string:
	for (int analogPin = 0; analogPin < 3; analogPin++)
	{
		int sensor = analogRead(analogPin);
		dataString += String(sensor);
		if (analogPin < 2)
		{
			dataString += ",";
		}
	}

	dataFile.println(dataString);

	// print to the serial port too:
	Serial.println(dataString);

	// The following line will 'save' the file to the SD card after every
	// line of data - this will use more power and slow down how much data
	// you can read but it's safer!
	// If you want to speed up the system, remove the call to flush() and it
	// will save the file only every 512 bytes - every time a sector on the
	// SD card is filled with data.
	dataFile.flush();

	// Take 1 measurement every 500 milliseconds
	delay(500);
}