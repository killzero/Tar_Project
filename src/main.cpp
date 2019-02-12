#include <Arduino.h>

#include <DS1307RTC.h>
#include <TimeLib.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

tmElements_t tm;
tmElements_t last_tm;

File myFile;
const int chipSelect = 10;

// ---------------------- about time ---------------------------------
void printTime();
void print2digits(int number);

void setTime();
const char *monthName[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

bool getTime(const char *str);
bool getDate(const char *str);
// --------------------------------------------------------------------

uint8_t moisture_pin[3] = {A4, A5, A6}; // moisture sensor pin
uint8_t solenoid_pin[3] = {5, 6, 7};    // solenoid pin

uint32_t valveTime;

void setup()
{
    Serial.begin(9600);
    for (int i = 0; i < 3; i++)
    {
        pinMode(moisture_pin[i], INPUT_PULLUP); // Declare pinmode of sensor
        pinMode(solenoid_pin[i], OUTPUT);       // Declare pinmode of solenoid
    }
    delay(1000); // delay to prepare to run
    setTime();

    if (RTC.read(tm))
    {
        // valveTime = tm.Second;
        valveTime = millis();
    }
}
/*
    When we took the readings from the dry soil, 
    then the sensor value was 550 and in the wet soil,
    the sensor value was 10
    */

bool valveOn[3] = {false, false, false}; // save state of valve

// set range of moisture limit
uint8_t _min[3] = {45, 45, 45};
uint8_t _max[3] = {55, 55, 55};

uint16_t moisture[3]; // Declare variable to keep measured value
int tenppTime = 0;
typedef unsigned long _time;
void loop()
{
    // set timer of work (millisecond)
    if (millis() - valveTime > 2000)
    {
        last_tm = tm;
        
        for (int i = 0; i < 1; i++)
        {
            moisture[i] = analogRead(moisture_pin[i]); // read sensor
            //Serial.print(moisture[i]);
            //Serial.print(" : ");
            moisture[i] = constrain(moisture[i], 0, 550);   // 550 - 10
            moisture[i] = map(moisture[i], 550, 0, 0, 100); // map value to percentage
            //Serial.println(moisture[i]);
            // printTime();
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
        valveTime = millis(); // reset timer
    }
}

void setTime()
{
    bool parse = false;
    bool config = false;

    // get the date and time the compiler was run
    if (getDate(__DATE__) && getTime(__TIME__))
    {
        parse = true;
        // and configure the RTC with this info
        if (RTC.write(tm))
        {
            config = true;
        }
    }
    while (!Serial)
        ; // wait for Arduino Serial Monitor
    delay(200);
    if (parse && config)
    {
        Serial.print("DS1307 configured Time=");
        Serial.print(__TIME__);
        Serial.print(", Date=");
        Serial.println(__DATE__);
    }
    else if (parse)
    {
        Serial.println("DS1307 Communication Error :-{");
        Serial.println("Please check your circuitry");
    }
    else
    {
        Serial.print("Could not parse info from the compiler, Time=\"");
        Serial.print(__TIME__);
        Serial.print("\", Date=\"");
        Serial.print(__DATE__);
        Serial.println("\"");
    }
}

bool getTime(const char *str)
{
    int Hour, Min, Sec;

    if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3)
        return false;
    tm.Hour = Hour;
    tm.Minute = Min;
    tm.Second = Sec;
    return true;
}

bool getDate(const char *str)
{
    char Month[12];
    int Day, Year;
    uint8_t monthIndex;

    if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3)
        return false;
    for (monthIndex = 0; monthIndex < 12; monthIndex++)
    {
        if (strcmp(Month, monthName[monthIndex]) == 0)
            break;
    }
    if (monthIndex >= 12)
        return false;
    tm.Day = Day;
    tm.Month = monthIndex + 1;
    tm.Year = CalendarYrToTm(Year);
    return true;
}

void printTime()
{
    if (RTC.read(tm))
    {
        Serial.print("Ok, Time = ");
        print2digits(tm.Hour);
        Serial.write(':');
        print2digits(tm.Minute);
        Serial.write(':');
        print2digits(tm.Second); // int
        Serial.print(", Date (D/M/Y) = ");
        Serial.print(tm.Day);
        Serial.write('/');
        Serial.print(tm.Month);
        Serial.write('/');
        Serial.print(tmYearToCalendar(tm.Year));
        Serial.println();
    }
    else
    {
        if (RTC.chipPresent())
        {
            Serial.println("The DS1307 is stopped.  Please run the SetTime");
            Serial.println("example to initialize the time and begin running.");
            Serial.println();
        }
        else
        {
            Serial.println("DS1307 read error!  Please check the circuitry.");
            Serial.println();
        }
    }
}

void print2digits(int number)
{
    if (number >= 0 && number < 10)
    {
        Serial.write('0');
    }
    Serial.print(number);
}