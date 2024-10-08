#include <SPI.h>              // SPI library
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_Sensor.h>  // Core sensor library
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789
#include <Adafruit_ADS1X15.h> // Hardware-specific library for ADS1x15
#include "Adafruit_BME680.h"  // Hardware-specific library for BME680
#include <Fonts/FreeSerif9pt7b.h> //Font to be used on the display
#include "SparkFun_BMA400_Arduino_Library.h" //Hardware-specific library for BMA400
#include <RCWL_1X05.h> //Hardware-specific library for Ultrasonic Sensor

//Pin Definitions
#define TFT_CS 38
#define TFT_RST -1
#define TFT_DC 48

#define LED_PIN 19
#define BUZZER_PIN 7

//Other Constants
#define BME_Addr 0x76
#define SEALEVELPRESSURE_HPA (1013.25)

//Location in memory to store SPI object and display object
//The SPI object is used by the display class to send data to display
SPIClass* fspi = NULL; //SPI
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

//Location In Memory to store Sensor Objects
Adafruit_BME680 bme;
Adafruit_ADS1015 ads;
BMA400 accelerometer;
RCWL_1X05 ultrasonic;

//Location In Memory to Store Sensor Results (Stored inside a Struct)

//Define Struct
struct SensorResults {
  //BME Results
  float T;
  float RH;
  float P;
  float G;
  float Alt;
  //BMA Results 
  float Xg;
  float Yg;
  float Zg;
  //ADC Results Raw
  int16_t Mic;
  int16_t EMF;
  int16_t Light;
  int16_t AIN;
  //ADC Results Converted to Volts
  float vMic;
  float vEMF;
  float vLight;
  float vAIN;
  //Ultrasonic Sensor Pulse TTR
  float USDistance;
};

//Declare Struct
SensorResults SensorData;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  Serial.println("Hello World!");

  //Init Buzzer and LED GPIO
  digitalWrite(LED_PIN, LOW); //Turn off before setting to output to prevent flickering at boot
  digitalWrite(BUZZER_PIN, LOW);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  //Init Display
  fspi = new SPIClass(FSPI);
  fspi->begin(36, 37, 35);
  tft.init(135, 240);
  tft.setRotation(3);

  //Print text to console to indicate it got here in the code
  Serial.println("Initialized");

  //Create Boot Image
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(80, 100);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.print("UofT MIE1050");
  delay(1000);

  //Init all sensors
  Wire.begin(16,15);   //I2C Bus #1 Init (For BME, BMA, and ADC)
  Wire1.begin(39, 40); //I2C Bus #2 Init (For Ultrasonic)

  //Init BME680
  if (!bme.begin(BME_Addr)) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
  }

  // Set up oversampling and filter for BME680
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  //Init ADC (ADS1015)
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
  }

  //Init BMA400
  if(accelerometer.beginI2C(BMA400_I2C_ADDRESS_DEFAULT) != BMA400_OK) {
    Serial.println("Error: BMA400 not connected, check wiring and I2C address!");
  }

  //Init Ultrasonic Sensor
  if (!ultrasonic.begin(&Wire1)) {
    Serial.println("Sensor not found. Check connections and I2C-pullups and restart sketch.");
  } else {
    Serial.println("Sensor ready, let's start\n\n");
    ultrasonic.setMode(RCWL_1X05::oneShot); // not really needed, it's the default mode
  }
}

//loop function runs over and over again forever
void loop() {
  readSensors();
  printSensors();
  updateDisplay();
  delay(800);
}

void readSensors(){
  //Read BME and store in struct
  if (! bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }
  SensorData.T = bme.temperature;
  SensorData.RH = bme.humidity;
  SensorData.P = bme.pressure/1000;
  SensorData.G = bme.gas_resistance/1000;
  SensorData.Alt = bme.readAltitude(SEALEVELPRESSURE_HPA);

  //Read BME and store in struct
  accelerometer.getSensorData();

  SensorData.Xg = accelerometer.data.accelX;
  SensorData.Yg = accelerometer.data.accelY;
  SensorData.Zg = accelerometer.data.accelZ;

  //Read ADC and store in Struct a raw value
  SensorData.Mic = ads.readADC_SingleEnded(0);
  SensorData.EMF = ads.readADC_SingleEnded(1);
  SensorData.Light = ads.readADC_SingleEnded(2);
  SensorData.AIN = ads.readADC_SingleEnded(3);
  
  //Convert Values to volts and store that
  SensorData.vMic = ads.computeVolts(SensorData.Mic);
  SensorData.vEMF = ads.computeVolts(SensorData.EMF);
  SensorData.vLight = ads.computeVolts(SensorData.Light);
  SensorData.vAIN = ads.computeVolts(SensorData.AIN);

  //Ultrasonic
  SensorData.USDistance = (float)(ultrasonic.read()*pow(10,6)/speedOfSoundUsedByI2Cmode);
  //SensorData.USDistance = ultrasonic.read();

  //Beep and flash LED to confirm sample taken
  digitalWrite(LED_PIN, HIGH);
 // tone(BUZZER_PIN, 600, 15);
  delay(50);
  digitalWrite(LED_PIN, LOW);

}

float ultrasonic_transfer(float temperature, float )
{
  return -1;

}

void printSensors(){
  //Print Sensor Values to Console
  Serial.println("-----------------------------------------------------------");

  Serial.printf("Temp:  %f °C\n", SensorData.T);
  Serial.printf("Hum:   %f %%\n", SensorData.RH);
  Serial.printf("Pres:  %f kPa\n", SensorData.P);
  Serial.printf("Gas:   %f KOhms\n", SensorData.G);
  Serial.printf("Alt:   %f m (Estimated)\n", SensorData.Alt);

  Serial.printf("Xg:    %f g\n", SensorData.Xg);
  Serial.printf("Yg:    %f g\n", SensorData.Yg);
  Serial.printf("Zg:    %f g\n", SensorData.Zg);

  Serial.printf("Mic:   %d (%f V)\n", SensorData.Mic, SensorData.vMic);
  Serial.printf("EMF:   %d (%f V)\n", SensorData.EMF, SensorData.vEMF);
  Serial.printf("Light: %d (%f V)\n", SensorData.Light, SensorData.vLight);
  Serial.printf("AIN:   %d (%f V)\n", SensorData.AIN, SensorData.vAIN);

  Serial.printf("US:    %f mm\n", SensorData.USDistance);
}

void updateDisplay(){
  char tempStringSorage[128] = {0}; //Used as a place for store strings that we created before sending to display buffer
  
  //Define an x value that is used fo drawing text in a consistant collumn
  const int COLLUMN_ONE_X = 0;
  const int COLLUMN_TWO_X = 120;

  //Constants to make code prettier
  const uint16_t TITLE_COLOUR = ST77XX_WHITE;
  const uint16_t VALUE_COLOUR = ST77XX_CYAN;

  //Fill screen with black
  tft.fillScreen(ST77XX_BLACK);
  //Configure text options
  tft.setTextWrap(false);
  tft.setFont(&FreeSerif9pt7b);

  //Draw all gridlines
  tft.drawFastVLine(COLLUMN_TWO_X-10, 0, 128, ST77XX_YELLOW);
  tft.drawFastHLine(COLLUMN_TWO_X-10, 68, 128, ST77XX_YELLOW);
  tft.drawFastHLine(0, 99, COLLUMN_TWO_X-10, ST77XX_YELLOW);

  //------Write All Text to Screen------

  /* GENERAL PATTERN
  tft.setCursor(x,y); //Set the virtual cursor to a place on the screen
  memset(tempStringSorage, 0, sizeof(tempStringSorage)); //Reset temp string storage so there is no overlap
  sprintf(tempStringSorage, "P: %3.1f kPa\n", SensorData.P); //Create a string basted on standard 'C string format specifiers'
  tft.print(tempStringSorage); //Print String to screen, this will handle drawing the slected font, in the selected colour, and moving the cursor for each character
  */

  //Write BME680 Values to Screen
  tft.setCursor(COLLUMN_ONE_X,14);
  tft.setTextColor(TITLE_COLOUR);
  tft.print("BME680\n");
  tft.setTextColor(VALUE_COLOUR);

  tft.setCursor(COLLUMN_ONE_X,32);
  sprintf(tempStringSorage, "T: %3.2f °C\n", SensorData.T);
  tft.print(tempStringSorage);

  tft.setCursor(COLLUMN_ONE_X,48);
  memset(tempStringSorage, 0, sizeof(tempStringSorage));
  sprintf(tempStringSorage, "H: %3.2f %%\n", SensorData.RH);
  tft.print(tempStringSorage);

  tft.setCursor(COLLUMN_ONE_X,64);
  memset(tempStringSorage, 0, sizeof(tempStringSorage));
  sprintf(tempStringSorage, "P: %3.1f kPa\n", SensorData.P);
  tft.print(tempStringSorage);

  tft.setCursor(COLLUMN_ONE_X,80);
  memset(tempStringSorage, 0, sizeof(tempStringSorage));
  sprintf(tempStringSorage, "G: %3.0fkOhm\n", SensorData.G);
  tft.print(tempStringSorage);

  tft.setCursor(COLLUMN_ONE_X,95);
  memset(tempStringSorage, 0, sizeof(tempStringSorage));
  sprintf(tempStringSorage, "A: %3.1f m\n", SensorData.Alt);
  tft.print(tempStringSorage);

  //USS to Screen
  tft.setCursor(COLLUMN_ONE_X,113);
  tft.setTextColor(TITLE_COLOUR);
  tft.print("Ultrasonic\n");
  tft.setTextColor(VALUE_COLOUR);
  tft.setCursor(COLLUMN_ONE_X,128);
  memset(tempStringSorage, 0, sizeof(tempStringSorage));
  sprintf(tempStringSorage, "%5.0f mm\n", SensorData.USDistance);
  tft.print(tempStringSorage);

  //Write BMA400 Values to Screen
  tft.setCursor(COLLUMN_TWO_X,83);
  tft.setTextColor(TITLE_COLOUR);
  tft.print("BMA400\n");
  tft.setTextColor(VALUE_COLOUR);

  tft.setCursor(COLLUMN_TWO_X,98);
  sprintf(tempStringSorage, "X: %2.3f g\n", SensorData.Xg);
  tft.print(tempStringSorage);

  tft.setCursor(COLLUMN_TWO_X,113);
  memset(tempStringSorage, 0, sizeof(tempStringSorage));
  sprintf(tempStringSorage, "Y: %2.3f g\n", SensorData.Yg);
  tft.print(tempStringSorage);

  tft.setCursor(COLLUMN_TWO_X,128);
  memset(tempStringSorage, 0, sizeof(tempStringSorage));
  sprintf(tempStringSorage, "Z: %2.3f g\n", SensorData.Zg);
  tft.print(tempStringSorage);

  //Write ADC Values to Screen
  tft.setCursor(COLLUMN_TWO_X,14);
  sprintf(tempStringSorage, "M: %2.3fV\n", SensorData.vMic);
  tft.print(tempStringSorage);

  tft.setCursor(COLLUMN_TWO_X,32);
  memset(tempStringSorage, 0, sizeof(tempStringSorage));
  sprintf(tempStringSorage, "E: %2.3fV\n", SensorData.vEMF);
  tft.print(tempStringSorage);

  tft.setCursor(COLLUMN_TWO_X,48);
  memset(tempStringSorage, 0, sizeof(tempStringSorage));
  sprintf(tempStringSorage, "L: %2.3fV\n", SensorData.vLight);
  tft.print(tempStringSorage);

  tft.setCursor(COLLUMN_TWO_X,64);
  memset(tempStringSorage, 0, sizeof(tempStringSorage));
  sprintf(tempStringSorage, "A: %2.3fV\n", SensorData.vAIN);
  tft.print(tempStringSorage);
}
