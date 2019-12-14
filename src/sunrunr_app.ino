#include "Adafruit_SSD1306.h"
#include <Adafruit_GFX.h>
#include <Adafruit_VEML6070.h>
#include <AssetTracker.h>

#define OLED_RESET D4
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

#if (SSD1306_LCDHEIGHT != 64)
    #error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

static const unsigned char logo16_glcd_bmp[] = { 
    0B00000000, 0B11000000,
    0B00000001, 0B11000000,
    0B00000001, 0B11000000,
    0B00000011, 0B11100000,
    0B11110011, 0B11100000,
    0B11111110, 0B11111000,
    0B01111110, 0B11111111,
    0B00110011, 0B10011111,
    0B00011111, 0B11111100,
    0B00001101, 0B01110000,
    0B00011011, 0B10100000,
    0B00111111, 0B11100000,
    0B00111111, 0B11110000,
    0B01111100, 0B11110000,
    0B01110000, 0B01110000,
    0B00000000, 0B00110000 };
    
Adafruit_VEML6070 UVTracker = Adafruit_VEML6070();
AssetTracker locationTracker = AssetTracker();
Adafruit_SSD1306 display(OLED_RESET);

int state = 0;
int indeX = 0;
String measArr[500];

//Global Variables
float LON = 0.0;
float LAT = 0.0;
float SPD = 0.0;
float TUV = 0.0;
bool FIX = false;
String timeString = "";
bool executeStateMachines = false;
float uvThreshold = 42.0;
time_t TIME;
//for the interrupt
bool activityStart = false;

typedef enum photon_state_enum{
    wait, measure, store, report, post, delay15,
} internal_state;

 //initialize first state
internal_state move_state = wait;

String stateName = "";


void setup() {
    //Sets correct time zone
    Time.zone(-5);
    Serial.begin(9600);
    
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    
    locationTracker.begin();
    locationTracker.gpsOn();
    
    UVTracker.begin(VEML6070_1_T);

    pinMode(D5, INPUT_PULLDOWN);
    
    //boot up display
    display.display();
    delay(2000);
    display.clearDisplay();
}

void loop() {
    switch (move_state){
        case wait:
            indeX = 0;
            stateName = "WAIT";
            updateOLED();
            move_state = (digitalRead(D5) == HIGH)? measure: wait;
            break;
            
        case measure:
            activityStart = true;
            stateName = "MEASURE";
            updateOLED();
            measureFunc();
            move_state = store;
            break;
            
        case store:
            stateName = "STORE";
            updateOLED();
            storeFunc();
            move_state = delay15;
            break;
            
        case delay15:
            stateName = "DELAY 1";
            updateOLED();
            {bool btnPress = false;
            for (int i=1; i < 16; i++){
                if(digitalRead(D5) == HIGH){
                    btnPress = true;
                    break;
                }
                delay(1000);
                stateName = String::format("DELAY %d", (i));
                updateOLED();
            }
            move_state = (btnPress)? report: measure;
            }
            break; 
            
        case report:
            Serial.println("Activity Stopped");
            activityStart = false;
            stateName = "REPORT";
            updateOLED();
            move_state = post;
            
            break;
            
        case post:
            for(int i = 0; i < 500; i++){
                Serial.println(String::format("%d:    %s", i, measArr[i]));
                delay(1000);
            }
            stateName = "POST";
            updateOLED();
            //NEED THE REPORT STATE
            move_state = wait;
            break;
    }
    delay(500);
}

void measureFunc(){
    //Get the location
    locationTracker.updateGPS(); 
    
    //Update Global Variables
    LON = locationTracker.readLonDeg();
    LAT = locationTracker.readLatDeg();
    SPD = locationTracker.getSpeed();
    FIX = locationTracker.gpsFix();
    TUV = UVTracker.readUV();
    TIME = Time.now();
    /*if(SPD == 0){
        delay(1000);
        stateName = measure;
    }
    else{
        stateName = store;
    }*/
}

void storeFunc(){
    String data = "";
    if (FIX) { //GPS Fixed
        data = String::format("{ \"lon\": \"%f\", \"lat\": \"%f\", \"GPS_speed\": \"%f\", \"uv\": \"%f\", \"time\": \"%s\" }", LON, LAT, SPD, TUV, (const char *)Time.format(TIME, TIME_FORMAT_DEFAULT));  
    }
    else {
        data = String::format("{ \"lon\": \"%f\", \"lat\": \"%f\", \"GPS_speed\": \"%f\", \"uv\": \"%f\", \"time\": \"%s\"  }", 999.999, 999.999, 999.999, TUV, (const char *)Time.format(TIME, TIME_FORMAT_DEFAULT));
    }
    //Where to we add string to the measArr
    measArr[indeX] = data;
    indeX++;
    
}

void updateOLED(){
    
    //Clear Display
    display.clearDisplay();
    
    //Text Settings
    display.setTextSize(1);
    display.setTextColor(WHITE);
    
    //Start laying out the output text prints 
    display.setCursor(0,0);
    display.println(stateName);
    display.setCursor(90,0);
    display.println((FIX)? "FIX":"NO FIX");
    
    display.setCursor(0, 16);
    display.println("Longitude:");
    display.setCursor(62, 16);
    display.println(LON);
    
    display.setCursor(0, 25);
    display.println("Latitude:");
    display.setCursor(58, 25);
    display.println(LAT);
    
    display.setCursor(0, 34);
    display.println("Speed:");
    display.setCursor(40, 34);
    display.println(SPD);
    
    display.setCursor(0, 43);
    display.println("UV:");
    display.setCursor(20, 43);
    display.println(TUV);
    
    display.setCursor(0, 52);
    display.println((activityStart)? "Activity": "No Activity");
    
    //Finally outputs all the text prints
    display.display();
}