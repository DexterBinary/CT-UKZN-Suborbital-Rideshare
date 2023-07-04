//Code for 2023-02-14 - Phoenix 1C - Cape Town - Suborbital - UKZN ASRI launch
//Uses a Accelerometer - KXTJ-1057, and a Barometer - SPL06-001
//Made by Declan Saul

//Libraries
#include <Arduino.h>
#include <Wire.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "kxtj3-1057.h"
#include "SPL06-007.h"

//Other Stuff
#define KXTJ3_DEBUG Serial
#define LOW_POWER

//Pins
#define SPI_MISO 12
#define SPI_MOSI 13
#define SPI_SCK 14
#define SD_CS 5

//I2C
#define I2C_SDA 26
#define I2C_SCL 27

double threshold = 2.5; // Threshold for the accelerometer to initiate the rest of the script (to stop from reading before the rocket is launched) in g's
bool hasLaunched = false;
double localPressure = 1017.0; // Local pressure in hPa (Cape Town South Africa)
String header = "Time,Pressure,Altitude,Temperature,Acell X,Acell Y,Acell Z,"; //Data Points included in the CSV
float   sampleRate = 6.25;  // HZ - Samples per second - 0.781, 1.563, 3.125, 6.25, 12.5, 25, 50, 100, 200, 400, 800, 1600Hz
uint8_t accelRange = 16;     // Accelerometer range = 2, 4, 8, 16g

KXTJ3 myIMU(0x0F); //IMU Address

SPIClass spi = SPIClass(HSPI); //Creates an object from class

//append to file
void write(String path, String data) {
  File file = SD.open(path, FILE_APPEND);
  if(!file) { return; }
  if(file.println(data)) { } else { }
  file.close();
}

//write to file (deletes original contents)
void writeHeader(String path, String header) {
  File file = SD.open(path, FILE_WRITE);
  if(!file) { return; }
  if(file.println(header)) { } else { }
  file.close();
}

//Timestamp message
String tm(String x ) {
  String r="";
  r+=
    x+
    " ["+
    String(millis())+
    "]";
  return r;
}

//Take Readings
void takeReadings(){

  myIMU.standby(false);

  String data = "";
  data +=
    String(millis())+","+
    String(get_pressure())+","+
    String(get_altitude(get_pressure(),localPressure))+","+
    String(get_temp_c())+","+
    String(myIMU.axisAccel(X))+","+
    String(myIMU.axisAccel(Y))+","+
    String(myIMU.axisAccel(Z));
  write("/data.csv", data);

  myIMU.standby(true);
}
void setup(){
  //Begin serial
  delay(5000);
  Serial.begin(115200);
  Serial.println(tm("Starting..."));

  //Starting the pins for the I2C
  Wire.setPins(26, 27);
  Wire.begin();

  // Initialize SD Card
  spi.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS); 
  if (!SD.begin(SD_CS, spi)) { 
    Serial.println(tm("SD Card initialization failed"));
    return;
  } else { 
    Serial.println(tm("SD Card initialized"));
    write("/log.txt", tm("SD Card initialized"));
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    Serial.println(tm("No SD Card present"));
    return; 
  } else { 
    Serial.println(tm("SD Card loaded"));
    write("/log.txt", tm("SD Card loaded"));
  }

  // Write the header to the CSV file
  writeHeader("/data.csv", header);
  Serial.println(tm("Wrote header to file"));
  write("/log.txt", tm("Wrote header to file"));

  //Start Sensors
  //IMU Start
  if( myIMU.begin(sampleRate, accelRange) != 0 )
  {
    Serial.print("Failed to initialize IMU.\n");
  }
  else
  {
    Serial.print("IMU initialized.\n");
  }
  myIMU.intConf(123, 1, 10, HIGH);
  //IMU End

  SPL_init(0x77); //Barometer
}


void loop() {
  myIMU.standby(false);
  delay(100);
    //Checks if the current acceleration is larger than the threshold or not
    //If currAcc > threshold then it will start taking readings
  if ((abs(myIMU.axisAccel(X)) > threshold or abs(myIMU.axisAccel(Y)) > threshold or abs(myIMU.axisAccel(Z)) > threshold) and hasLaunched == false) {
    write("/log.txt", "Rocket Launch Detected at " + String(millis()/60000) + " minutes");
    hasLaunched = true;
  }

  if (hasLaunched == true) {
    write("/log.txt", tm("Taking readings at"));
    takeReadings();
    write("/log.txt", tm("Finished taking readings at"));
  }
  
  myIMU.standby(true);
}
