uint8_t moisture_pin[3] = {A5, A6, A7}; // moisture sensor pin
uint8_t solenoid_pin[3] = {5, 6, 7};    // solenoid pin

uint32_t currentTime = millis(); // timer value

void setup()
{
    Serial.begin(9600);
    for (int i = 0; i < 3; i++)
    {
        pinMode(moisture_pin[i], INPUT_PULLUP); // Declare pinmode of sensor
        pinMode(solenoid_pin[i], OUTPUT);       // Declare pinmode of solenoid
    }
    delay(1000); // delay to prepare to run
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
void loop()
{
    // set timer of work (millisecond)
    if (millis() - currentTime > 500)
    {
        for (int i = 0; i < 3; i++)
        {
            moisture[i] = analogRead(moisture_pin[i]);      // read sensor
            moisture[i] = constrain(moisture[i], 0, 550);   // 550 - 10
            moisture[i] = map(moisture[i], 550, 0, 0, 100); // map value to percentage
            Serial.println(moisture[i]);
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
        currentTime = millis(); // reset timer
    }
}
