//for help look at: https://github.com/Starmbi/hp_BH1750/wiki
#include <Arduino.h>
#include <hp_BH1750.h>  //  include the library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
hp_BH1750 BH1750;       //  create the sensor

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  bool avail = BH1750.begin(BH1750_TO_GROUND);// init the sensor with address pin connetcted to ground
  // result (bool) wil be be "false" if no sensor found
  if (!avail) {
    Serial.println("No BH1750 sensor found!");
    while (true) {};                                        
  }
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
	display.setTextSize(2);
	display.setTextColor(SSD1306_WHITE);
}

void loop()
{
  
  // put your main code here, to run repeatedly:
  BH1750.start();   //starts a measurement
  float lux=BH1750.getLux();  //  waits until a conversion finished
  
  display.clearDisplay();
  display.setCursor(0, 0);
	display.print("lux: ");
	display.print(lux);

  if(lux < 50){
      Serial.print(lux);
      Serial.println(" TRUE");        
      display.setCursor(0, 18);
      display.print("led: TRUE");
    } else {
      Serial.print(lux);
      Serial.println(" FALSE");
      display.setCursor(0, 18);
      display.print("led: FALSE");
  }
  display.display();
}