#include <Arduino.h>

#include <amt1001_ino.h>
#include <DS1307RTC.h>
#include <TimeLib.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

tmElements_t tm;

File myFile;
const int chipSelect = 10;

#define airTemp_pin A2
#define airHumid_pin A3
// ---------------------- about time ---------------------------------
void printTime();
void print2digits(int number);

void setTime();
const char *monthName[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

bool getTime(const char *str);
bool getDate(const char *str);
void init_SD();
// --------------------------------------------------------------------

uint8_t moisture_pin[3] = {A4, A5, A6}; // moisture sensor pin
uint8_t solenoid_pin[3] = {5, 6, 7};    // solenoid pin

uint32_t valveTime, logTime;
bool valveOn[3] = {false, false, false}; // save state of valve

// set range of moisture limit
uint8_t _min[3] = {45, 45, 45};
uint8_t _max[3] = {55, 55, 55};

uint16_t moisture[3]; // Declare variable to keep measured value
uint16_t lastMinute = 0;
void controlMoisture(uint16_t _time);
void writeLog();
uint16_t getAirTemp();
uint16_t getAirHumid();
// --------------------------------------------------------------------

void setup()
{
    Serial.begin(9600);

    pinMode(SS, OUTPUT);
    if (!SD.begin(10, 11, 12, 13))
    {
        Serial.println("Card failed, or not present");
        // don't do anything more:
        while (1)
            ;
    }
    Serial.println("card initialized.");
    for (int i = 0; i < 3; i++)
    {
        pinMode(moisture_pin[i], INPUT_PULLUP); // Declare pinmode of sensor
        pinMode(solenoid_pin[i], OUTPUT);       // Declare pinmode of solenoid
    }

    pinMode(airTemp_pin, INPUT);
    pinMode(airHumid_pin, INPUT);

    delay(1000); // delay to prepare to run
    setTime();
    init_SD();
    if (RTC.read(tm))
    {
        // valveTime = tm.Second;
        valveTime = millis();
    }
}

void loop()
{
    writeLog();
    controlMoisture(500);
}

void controlMoisture(uint16_t _time)
{
    /*
    When we took the readings from the dry soil, 
    then the sensor value was 550 and in the wet soil,
    the sensor value was 10
    */
    // set timer of work (millisecond)
    if (millis() - valveTime > _time)
    {
        for (int i = 0; i < 1; i++)
        {
            moisture[i] = analogRead(moisture_pin[i]); // read sensor
            //Serial.print(moisture[i]);
            //Serial.print(" : ");
            moisture[i] = constrain(moisture[i], 0, 550);   // 550 - 10
            moisture[i] = map(moisture[i], 550, 0, 0, 100); // map value to percentage

            // Serial.println(moisture[i]);
            // printTime();

            // solenoid control
            if (_min[i] > moisture[i])
            {
                digitalWrite(solenoid_pin[i], 1);
                valveOn[i] = true;
            }
            else if (valveOn[i] && moisture[i] > _max[i])
            {
                digitalWrite(solenoid_pin[i], 0);
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

void init_SD()
{
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    myFile = SD.open("log.csv", FILE_WRITE);

    // if the file opened okay, write to it:
    if (myFile)
    {
        Serial.print("Writing to test.txt...");
        // myFile.println("hour,minute,day,mount,year");
        // close the file:

        myFile.close();
        Serial.println("SD is on.");
    }
    else
    {
        // if the file didn't open, print an error:
        Serial.println("can't open log.csv");
    }
}

void writeLog()
{
    if (tm.Minute % 5 == 0 && tm.Minute != lastMinute)
    {
        myFile = SD.open("log.csv", FILE_WRITE);
        if (myFile)
        {
            Serial.print("Writing log.csv ...");
            myFile.print(tm.Hour);
            myFile.print(",");
            myFile.print(tm.Minute);
            myFile.print(",");
            myFile.print(tm.Day);
            myFile.print(",");
            myFile.print(tm.Month);
            myFile.print(",");
            myFile.print(tmYearToCalendar(tm.Year));
            myFile.print(",");
            myFile.print(moisture[0]);
            myFile.print(",");
            myFile.print(moisture[1]);
            myFile.print(",");
            myFile.print(moisture[2]);
            // myFile.print(",");
            // myFile.print(getAirTemp());
            // myFile.print(",");
            // myFile.print(getAirHumid());

            // close the file:
            myFile.close();
        }
        else
        {
            Serial.println("can't open log.csv");
        }
        lastMinute = tm.Minute;
    }
}

uint16_t getAirTemp()
{
    // Get Temperature
    uint16_t temperature = analogRead(airTemp_pin);
    temperature = amt1001_gettemperature(temperature);
    return temperature;
}

uint16_t getAirHumid()
{
    // Get Humidity
    uint16_t humidity = analogRead(airHumid_pin);
    double volt = (double)humidity * (5.0 / 1023.0);
    humidity = amt1001_gethumidity(volt);
    return humidity;
}