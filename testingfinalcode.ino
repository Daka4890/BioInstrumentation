#include <SPI.h>
#include <SoftwareSerial.h>
#include "SparkFun_Si7021_Breakout_Library.h"
#include <Wire.h>

SoftwareSerial lcd(2, 3);         // LCD pin initialization
SI7021 myHumidity;                // SI7021 object initialization

const int sampleRate = 5;         // Sample rate in Minutes
float bioimpedance = 0.0;         // Global variable for bioimpedance
const int bufferSize = 5;         // Size of the buffer to store bioZ readings
float bioZBuffer[bufferSize];     // Buffer to store bioZ readings
const int tewlSize = 12;
float tewlArray[tewlSize];
int timer;

int count = 0;                    // counter to help keep track of time

//Sensor's memory register addresses:
const int STATUS = 0x01;   // Read Only 
const int EN_INT = 0x02; 
const int EN_INT2 = 0x03;
const int MNGR_INT = 0x04;
const int MNGR_DYN = 0x05;
const int SW_RST = 0x08;   // Write Only
const int SYNCH = 0x09;    // Write Only
const int FIFO_RST = 0x0A; // Write Only
const int INFO = 0x0F;     // Read Only
const int CNFG_GEN = 0x10;
const int CNFG_CAL = 0x12;
const int CNFG_EMUX = 0x14;
const int CNFG_ECG = 0x15;
const int CNFG_BMUX = 0x17;
const int CNFG_BIOZ = 0x18;
const int CNFG_PACE = 0x1A;
const int CNFG_RTOR1 = 0x1D;
const int CNFG_RTOR2 = 0x1E;
const int BIOZ_FIFO = 0x23;

const int dataReadyPin = 9;
const int chipSelectPin = 10;

const float V_REF = 1.0;                // V_REF in volts
const float BIOZ_CGMAG = 8 * 1e-6;      // 8 microamp current generator
const float BIOZ_GAIN = 20;             // 20V/V Gain

byte dataBuffer[3]; // Buffer to store received data

void setup() {
  Serial.begin(9600);

  //startup SPI interface
  SPI.begin();
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

  //Initialize other pins for SPI communication
  pinMode(dataReadyPin, INPUT);
  pinMode(chipSelectPin, OUTPUT);

  //Configure MAX300001 by writing registers
  writeRegister(EN_INT, 0x00, 0x00, 0x03);
  delay(1);
  writeRegister(EN_INT2, 0x00, 0x00, 0x03);
  delay(1);
  writeRegister(MNGR_INT, 0x7B, 0x00, 0x04);
  delay(1);
  writeRegister(MNGR_DYN, 0x3F, 0xFF, 0xFF);
  delay(1);
  writeRegister(FIFO_RST, 0x00, 0x00, 0x00);
  delay(1);
  writeRegister(CNFG_GEN, 0x04, 0x00, 0x2B);
  delay(1);
  writeRegister(CNFG_CAL, 0x00, 0x48, 0x00);
  delay(1);
  writeRegister(CNFG_EMUX, 0x30, 0x00, 0x00);
  delay(1);
  writeRegister(CNFG_ECG, 0x80, 0x50, 0x00);
  delay(1);
  writeRegister(CNFG_BMUX, 0x00, 0x30, 0x40);
  delay(1);
  writeRegister(CNFG_BIOZ, 0x61, 0x04, 0x10);
  delay(1);
  writeRegister(CNFG_PACE, 0x00, 0x00, 0x55);
  delay(1);
  writeRegister(CNFG_RTOR1, 0x3F, 0x23, 0x00);
  delay(1);
  writeRegister(CNFG_RTOR2, 0x20, 0x24, 0x00);

  // give the sensor time to set up:
  delay(100);

  //startup LCD
  lcd.begin(9600);
  Wire.begin();

  //make sure count = 0
  count = 0;

  // making sure we are reading humidity
  while (myHumidity.begin() == false)
  {
    Serial.println("Sensor not found. Please check wiring. Freezing...");
    while (true)
      ;
  }

  timer = 60; // initialize 300 second countdown timer
}

void loop() {
  clearDisplay();
  if (timer <= 0)
  {
    clearDisplay();
    timer = 60;
    lcd.print("DRINK WATER!!!\nDRINK WATER!!!");
    Serial.println("DRINK WATER!!!\nDRINK WATER!!!");
    delay(5000);
  }
  float humidity = myHumidity.getRH();
  float temp = myHumidity.getTemperature();
  float tewl = waterLoss2(temp, humidity);

  if (tewl > 3.23)
  {
    timer--;
  }
  
  lcd.print("Countown to\nDrink H20: ");
  Serial.println("Countown to\nDrink H20: ");
  displayTime(timer);

  delay(3000);
  timer = timer - 3;

  clearDisplay();
  lcd.print("T: ");
  lcd.print(temp*9/5+32);
  lcd.print("F       RH: ");
  lcd.print(humidity);
  lcd.print("% ");

  Serial.print("T: ");
  Serial.print(temp*9/5+32);
  Serial.print("F       RH: ");
  Serial.print(humidity);
  Serial.print("% ");
  
  displayTime(timer);
  delay(3000);
  timer = timer - 3;
  clearDisplay();
  processBiozData();
  lcd.print("\nTEWL:");
  lcd.print(tewl);
  lcd.print("  ");

  Serial.print("\nTEWL:");
  Serial.print(tewl);
  Serial.print("  ");
  
  displayTime(timer);
  delay(3000);
  timer = timer - 3;
}

void writeRegister(byte regAddress, byte data1, byte data2, byte data3) {
  regAddress <<= 1;
  
  digitalWrite(chipSelectPin, LOW);
  SPI.transfer(regAddress);
  SPI.transfer(data1);
  SPI.transfer(data2);
  SPI.transfer(data3);
  digitalWrite(chipSelectPin, HIGH);
}

void readRegister(byte regAddress) {
  regAddress <<= 1;
  regAddress |= 0x01;

  digitalWrite(chipSelectPin, LOW);
  SPI.transfer(regAddress);
  
  for (int i = 0; i < 3; i++) {
    dataBuffer[i] = SPI.transfer(0x00);
  }
  
  digitalWrite(chipSelectPin, HIGH);
}

String byteToBinary(byte value) {
  String binaryString = "";
  for (int i = 7; i >= 0; i--) {
    binaryString += ((value >> i) & 1) ? '1' : '0';
  }
  return binaryString;
}

void processBiozData() {
  writeRegister(FIFO_RST, 0x00, 0x00, 0x00);
  delay(100);
  readRegister(BIOZ_FIFO);

  byte lastThreeBits = dataBuffer[2] & 0b00000111;

  if (lastThreeBits == 0b000) {
    int biozData = (dataBuffer[0] << 12) | (dataBuffer[1] << 4) | ((dataBuffer[2] & 0xF0) >> 4);

    if (biozData & 0x80000) {
      biozData = (biozData ^ 0xFFFFF) + 1;
    }

    bioimpedance = biozData * V_REF / (pow(2,19) * BIOZ_CGMAG * BIOZ_GAIN);

    char bioimpedanceStr[10];
    dtostrf(bioimpedance, 6, 2, bioimpedanceStr);

    lcd.print("BioZ:");
    lcd.println(bioimpedanceStr);
  } else {
    lcd.println("Invalid data");
    writeRegister(FIFO_RST, 0x00, 0x00, 0x00);
  }
}

void clearDisplay() {
  lcd.write(0xFE);
  lcd.write(0x01);
  Serial.print("\n\n\n\n\n\n\n\n\n\n\n");
}

float waterLoss3(float temp){
    float tewl = 8.05 * pow(2.71828,.1238 * (temp - 25)); // temp is in celcius
    return tewl;
}

float waterLoss2(float temp, float humidity){
    float tewl = 7.8381 * pow(2.71828,.1106*(temp-25) - .0042 * (humidity - 65));
    return tewl;
}

void displayTime(int totalSeconds) {
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;

  if(minutes < 10) {
    Serial.print("0");
    lcd.print("0");
  }
  Serial.print(minutes);
  Serial.print(":");
  lcd.print(minutes);
  lcd.print(":");

  if(seconds < 10) {
    Serial.print("0");
    lcd.print("0");
  }
  Serial.println(seconds);
  lcd.println(seconds);
}
