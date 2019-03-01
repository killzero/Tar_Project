#include <Arduino.h>
#include <amt1001_ino.h>

#define tempPin A0
#define humPin A1
void setup()
{
    Serial.begin(9600);
    pinMode(tempPin, INPUT);
    pinMode(humPin, INPUT);
}

void loop()
{
    // Get Temperature
    uint16_t temperature = analogRead(tempPin);
    uint16_t temperature = amt1001_gettemperature(temperature);
    Serial.println(temperature);

    // Get Humidity
    uint16_t humidity = analogRead(humPin);
    double volt = (double)humidity * (5.0 / 1023.0);
    uint16_t humidity = amt1001_gethumidity(volt);
    Serial.println(humidity);
}