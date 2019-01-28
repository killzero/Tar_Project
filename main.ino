uint32_t currentTime = millis(); // timer value
uint8_t moisture[3] = {5, 6, 7}; // moisture sensor pin
uint8_t solenoid = A8;            // solenoid pin
void setup()
{
    for (int i = 0; i < 3; i++)
    {
        // Declare pinmode of sensor
        pinMode(moisture[i], INPUT);
    }
    pinMode(solenoid, OUTPUT); // Declare pinmode of solenoid
    delay(1000);               // delay to prepare to run
}
/*
    When we took the readings from the dry soil, 
    then the sensor value was 550 and in the wet soil,
    the sensor value was 10
    */

bool valveOn = false;   // save state of valve
void loop()
{
    // set range of moisture limit
    uint8_t min = 45, max = 50;

    // set timer of work (millisecond)
    if (millis() - currentTime > 200)
    {
        uint16_t temp[3];   // Declare variable to keep measured value
        uint16_t minTemp;
        for (int i = 0; i < 3; i++)
        {
            temp[i] = analogRead(moisture[i]);          // read sensor
            temp[i] = map(temp[i], 550, 10, 0, 100);    // map value to percentage

            // Find the minimum value
            minTemp = min(temp[0], temp[1]);
            minTemp = min(minTemp, temp[2]);

            // solenoid control
            if (min > minTemp)
            {
                digitalWrite(solenoid, 1);
                valveOn = true;
            }
            else if (valveOn && minTemp > max)
            {
                digitalWrite(solenoid, 0);
                valveOn = false;
            }
        }
        currentTime = millis(); // reset timer
    }
}
