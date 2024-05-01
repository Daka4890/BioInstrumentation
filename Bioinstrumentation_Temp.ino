#include <SoftwareSerial.h>
#include "SparkFun_Si7021_Breakout_Library.h" //http://librarymanager/All#SparkFun_Si7021
#include <Wire.h>

SoftwareSerial lcd(2, 3);
SI7021 myHumidity;

int count;

void setup()
{
  Serial.begin(9600);
  lcd.begin(9600);
  Wire.begin();

  count = 0;

  while (myHumidity.begin() == false)
  {
    Serial.println("Sensor not found. Please check wiring. Freezing...");
    while (true)
      ;
  }
}

void loop()
{
  // Measure Relative Humidity from the Si7021

  if (count == 2){
    clearDisplay();
    lcd.print("Get some water");
    Serial.print("Get some water");
    delay(2000);
    count = 0;
  }

  float humidity = myHumidity.getRH();

  Serial.print("Humidity:");
  Serial.print(humidity, 1);
  Serial.print("%, ");
  Serial.print(count);

  // Measure Temperature from the Si7021
  float tempF = myHumidity.getTemperatureF();

  Serial.print("Temperature:");
  Serial.print(tempF, 2);
  Serial.print("F, ");

  // Display on LCD
  clearDisplay();
  lcd.print("Temp: ");
  lcd.print(tempF);
  lcd.print("F  Humidity: ");
  lcd.print(humidity);
  lcd.print("%");

  delay(10000); // 10-second delay between readings

  count = count + 1;
}

void clearDisplay()
{
  lcd.write(0xFE);  // send the special command
  lcd.write(0x01);  // send the clear screen command
}
