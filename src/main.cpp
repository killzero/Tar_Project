#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <amt1001_ino.h>
#include <DS1307RTC.h>
#include <TimeLib.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
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
uint8_t solenoid_pin[3] = {5, 6, 7};    // 2 leg solenoid pin
bool relayState[3] = {0, 0, 0};
uint32_t valveTime;
uint32_t _controlTime, logTime, lcdTime;
bool valveOn[3] = {false, false, false}; // save state of valve

// set range of moisture limit
uint8_t _min[3] = {45, 45, 45};
uint8_t _max[3] = {55, 55, 55};

uint16_t moisture[3]; // Declare variable to keep measured value

uint16_t lastMinute = 0;
void controlMoisture(uint16_t interval);
void writeLog();
void showLCD(uint16_t interval);
float getAirTemp();
uint16_t getAirHumid();

// --------------------------------------------------------------------

void setup()
{
    Serial.begin(9600);
    lcd.init(); // initialize the lcd
    lcd.backlight();

    pinMode(SS, OUTPUT);
    if (!SD.begin(10, 11, 12, 13))
    {
        Serial.println("Card failed, or not present");
        // don't do anything more:
        while (1)
            ;
    }
    else
        Serial.println("card initialized.");

    // init_SD();
    for (int i = 0; i < 3; i++)
    {
        pinMode(moisture_pin[i], INPUT_PULLUP); // Declare pinmode of sensor
        pinMode(solenoid_pin[i], OUTPUT);       // Declare pinmode of solenoid
    }

    pinMode(airTemp_pin, INPUT);
    pinMode(airHumid_pin, INPUT);

    delay(1000); // delay to prepare to run
    setTime();

    if (RTC.read(tm))
    {
        _controlTime = millis();
        logTime = millis();
        lcdTime = millis();
    }
}

void loop()
{
    controlMoisture(500);
    showLCD(1000);
    writeLog();
}

void controlMoisture(uint16_t interval)
{
    // set timer of work (millisecond)
    if (millis() - _controlTime > interval)
    {
        for (int i = 0; i < 3; i++)
        {
            moisture[i] = analogRead(moisture_pin[i]); // read sensor
            moisture[i] = constrain(moisture[i], 0, 550);   // 550 - 10
            moisture[i] = map(moisture[i], 550, 0, 0, 100); // map value to percentage

            printTime();
            bool allValve = false;
            for (int i = 0; i < 3; i++)
            {
                allValve = allValve || valveOn[i];
            }

            uint32_t _Vinterval = millis() - valveTime;
            /* *************** solenoid control ******************* */
            if ((_min[i] > moisture[i]) && !allValve && _Vinterval > 122000)
            { // moisture LOW and valve OFF     // rested
                digitalWrite(solenoid_pin[i], 1);
                valveOn[i] = true;
                valveTime = millis();
            }
            else if (moisture[i] > _max[i] && valveOn[i])
            { // moisture HIGH and valve ON
                digitalWrite(solenoid_pin[i], 0);
                valveOn[i] = false;
            }
            else
            {
                // moisture HIGH and valve OFF
                // moisture LOW and valve ON
                if (_Vinterval > 2000)
                { // restting
                    digitalWrite(solenoid_pin[i], 0);
                    valveOn[i] = false;
                }
            }
        }
        _controlTime = millis(); // reset timer
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

void writeLog()
{
    if (millis() - logTime > 60000 && RTC.read(tm))
    {
        myFile = SD.open("log02.csv", FILE_WRITE);
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
            for (uint8_t i = 0; i < 3; i++)
            {
                myFile.print(",");
                myFile.print(moisture[i]);
            }
            for (uint8_t i = 0; i < 3; i++)
            {
                myFile.print(",");
                myFile.print(valveOn[i]);
            }
            Serial.println("Writing log02.csv ...");
            String _temp = String(tm.Hour) + ',' + String(tm.Minute) + ',' + String(tm.Day);
            _temp += ',' + String(tm.Month) + ',' + tmYearToCalendar(tm.Year);
            Serial.println(_temp);
            myFile.print(_temp);
            delay(10);
            _temp = "";
            for (uint8_t i = 0; i < 3; i++)
            {
                _temp += ',' + String(moisture[i]);
            }

            _temp += ',' + String(getAirTemp()) + ',' + String(getAirHumid());

            myFile.println(_temp);

            // close the file:
            myFile.close();
        }
        else
        {
            Serial.println("can't open log.csv");
        }
        logTime = millis();
    }
}

float getAirTemp()
{
    // Get Temperature
    float temperature = analogRead(airTemp_pin);
    temperature = temperature * 500 / 1023; // * 5 / 1023 * 10080 / 0.8;
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

void showLCD(uint16_t interval)
{
    if (millis() - lcdTime > interval)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("moi_1:");
        lcd.print(moisture[0]);
        lcd.setCursor(8, 0);
        lcd.print("moi_2:");
        lcd.print(moisture[1]);
        lcd.setCursor(0, 1);
        lcd.print("moi_3:");
        lcd.print(moisture[2]);
        lcd.setCursor(8, 1);
        lcd.print("Air:");
        lcd.print(getAirHumid());
        lcd.setCursor(0, 2);
        lcd.print("Temp:");
        lcd.print(getAirTemp());
        lcd.setCursor(0, 3);
        lcd.print(tm.Day);
        lcd.write('/');
        lcd.print(tm.Month);
        lcd.write('/');
        lcd.print(tmYearToCalendar(tm.Year));

        lcdTime = millis();
    }
}
