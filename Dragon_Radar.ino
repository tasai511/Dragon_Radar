#include <Arduino_GFX_Library.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LSM303.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "JpegClass.h"
#include <ESP_EEPROM.h>


//------------------------
//   Customize Settings
//------------------------
#define BK_Fill 0x0ac4
#define BK_RED 0xa065
#define BK_ORANGE 0xbae1
#define BK_GREEN 0x0962
const char max_shuffle = 7;       //Max number of "Shuffle"
const int dist_range = 480;       //Max distance of random Dragon Ball allocation (M)
const int scope = 40;             //Display range of "Zoom" mode (M)
const int sleep_timer = 20000;    //Idle timer to sleep (msec)
const int get_range = 5;          //Distance to get Dragon Ball (M)
float location_lng = 139.767135527359;      //Home Position for Demo
float location_lat =  35.681366901497974;   //Home Position for Demo


//------------------------
//      Definitions
//------------------------
#define BUTTON 2
#define TFT_CS 15
#define TFT_DC 0
#define TFT_RST 12
#define TFT_BL 16
Arduino_DataBus *bus = new Arduino_ESP8266SPI(TFT_DC, TFT_CS);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RST, 0, true);
TinyGPSPlus gps;
const int RXPin = 3, TXPin = 1;
const int GPSBaud = 9600;
SoftwareSerial ss(RXPin, TXPin);
LSM303 compass;
const float pi = atan(1)*4;                                    //Pi (3.1415926..)
const float length_y = 6356.752*2*pi/36000000*1000;            //Distance M at 0.1 second of latitude
const unsigned int small_x[7] = {129,125,117,110,110,117,125}; //Display of collected Dragon Balls
const unsigned int small_y[7] = {119,127,129,123,115,109,111}; //Display of collected Dragon Balls


//------------------------
//  Initialize Variables
//------------------------
int scale = dist_range;
float DB_x[7];
float DB_y[7];
float Origin_x[7];              //Reference X-coordinate
float Origin_y[7];              //Reference Y-coordinate
float Origin_t[7];              //Reference angle θ
float Origin_h[7];              //Reference hypotenuse
float Radar_x[7];               //X-coordinate on Radar display
float Radar_y[7];               //Y-coordinate on Radar display
float Radar_t[7];               //Angle θ on Radar display
float length_x;                 //Distance M at 0.1 second of latitude
float Distance_x[7];            //Distance to X coordinate M
float Distance_y[7];            //Distance to Y coordinate M
unsigned int Distance[7];       //Distance to Dragon Ball M
unsigned int farthest_Distance; //Distance to the farthest dragon ball
unsigned int farthest_DB;       //Farthest Dragon Ball Number
unsigned int nearest_Distance;  //Distance to the nearest dragon ball離
unsigned int nearest_DB;        //Nearest Dragon Ball Number
unsigned char collection[7];    //List of collected Dragon Ball
bool display_mode;
bool Button_State;
bool Button_Flag;
unsigned long Button_Start;
unsigned long Last_Button;
unsigned char shuffle_count;
float heading;


//------------------------
//       Functions
//------------------------
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
      ButtonRead();
  } while (millis() - start < ms);
}

static JpegClass jpegClass;
static int jpegDrawCallback(JPEGDRAW *pDraw) {
  gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}

void fpm_wakup_cb(void)
{
  gpio_pin_wakeup_disable();
  wifi_fpm_close();
}

void start_light_sleep() {
  digitalWrite(TFT_BL, LOW);
  WiFi.mode(WIFI_OFF);
  wifi_set_opmode_current(NULL_MODE); 
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T); 
  wifi_fpm_open();
  gpio_pin_wakeup_enable(BUTTON, GPIO_PIN_INTR_LOLEVEL);
  wifi_fpm_set_wakeup_cb(fpm_wakup_cb);
  wifi_fpm_do_sleep(0xFFFFFFF);
  delay(10);
}

float calcOriginTheta(float x, float y) {
  float result = atan2(y, x);
  return result;
}

float calcOriginHypo(float x, float y) {
  float result = sqrt(pow(x, 2) + pow(y, 2));
  return result;
}

float calcTargetTheta(float theta, float head) {
  float result = theta + head/180*pi;
  return result;
}

float calcTargetX(float theta, float hypo) {
  float result = hypo * cos(theta);
  return result;
}

float calcTargetY(float theta, float hypo) {
  float result = hypo * sin(theta);
  return result;
}

void draw_grid() {
  int Power = analogRead(A0);
  if (Power != 1024) {
    gfx->drawCircle(120,120,119,RED);
  } else {
    gfx->drawCircle(120,120,119,BK_Fill);
  }
  for (int i=0; i<=6; i++) {
    if (collection[i] == 0) {
      gfx->fillCircle(small_x[i], small_y[i], 2, BK_Fill);
    }
  }
  if (display_mode == 0) {
    for (int i=14; i<=224; i=i+15) {
      gfx->drawFastVLine(i, 0, 240, BLACK);
      gfx->drawFastHLine(0, i, 240, BLACK);
    }
  } else if (display_mode == 1) {
    for (int i=29; i<=209; i=i+30) {
      gfx->drawFastVLine(i, 0, 240, BLACK);
      gfx->drawFastHLine(0, i, 240, BLACK);
    }
  }
  gfx->fillTriangle(114, 123, 119, 113, 125, 123, RED);
  for (int i=0; i<=6; i++) {
    if (collection[i] == 1) {
      gfx->fillCircle(small_x[i], small_y[i], 2, YELLOW);
    }
  }
}

void writeDistance(int color) {
  int line_x; int line_y;
  int cursor_x; int cursor_y;
  int Target;
  int Range;
  int Distance;
  if (display_mode == 0) {
    Target = farthest_DB;
    Range = scale;
    Distance = farthest_Distance;
  } else {
    Target = nearest_DB;
    Range = scope;
    Distance = nearest_Distance;
  }
  if (Distance <= Range) {
    if (Radar_x[Target]<119 && Radar_y[Target]<119) {
      line_x = 5; line_y = 10;
      cursor_x = line_x +2; cursor_y = line_y +5;
    } else if (Radar_x[Target]>=119 && Radar_y[Target]>=119) {
      line_x = -5; line_y = -10;
      cursor_x = line_x -18; cursor_y = line_y -10;
    } else if (Radar_x[Target]>=119 && Radar_y[Target]<119) {
      line_x = -5; line_y = 10;
      cursor_x = line_x -18; cursor_y = line_y +5;
    } else if (Radar_x[Target]<119 && Radar_y[Target]>=119) {
      line_x = 5; line_y = -10;
      cursor_x = line_x +2; cursor_y = line_y -10;
    }
    gfx->drawCircle(Radar_x[Target], Radar_y[Target], 5, color);
    gfx->drawLine(Radar_x[Target], Radar_y[Target], Radar_x[Target]+line_x, Radar_y[Target]+line_y, color);
    gfx->drawLine(Radar_x[Target]+line_x, Radar_y[Target]+line_y, Radar_x[Target]+line_x*5, Radar_y[Target]+line_y, color);
    gfx->setCursor(Radar_x[Target]+cursor_x, Radar_y[Target]+cursor_y);
    gfx->setTextColor(color);
    gfx->print(String(Distance) + "m");
  }
}

void getDB(int stars) {
  char* JPEG_FILENAME;
  if (stars == 0) { JPEG_FILENAME = "/1_Star.jpeg"; }
  else if  (stars == 1) { JPEG_FILENAME =  "/2_Star.jpeg"; }
  else if  (stars == 2) { JPEG_FILENAME =  "/3_Star.jpeg"; }
  else if  (stars == 3) { JPEG_FILENAME =  "/4_Star.jpeg"; }
  else if  (stars == 4) { JPEG_FILENAME =  "/5_Star.jpeg"; }
  else if  (stars == 5) { JPEG_FILENAME =  "/6_Star.jpeg"; }
  else { JPEG_FILENAME =  "/7_Star.jpeg"; }
  jpegClass.draw(&LittleFS, JPEG_FILENAME, jpegDrawCallback, true, -2, -2, 482, 482);
}

void Shenron() {
  while (1) {
    for (int i=0; i<=6; i++) {
      gfx->fillCircle(small_x[i], small_y[i], 5, 0xfea0);
      gfx->fillCircle(small_x[i], small_y[i], 2, YELLOW);
    }
    delay(600);
    for (int j=0; j<=6; j++) {
      gfx->fillCircle(small_x[j], small_y[j], 5, BK_Fill);
      gfx->fillCircle(small_x[j], small_y[j], 2, YELLOW);
    }
    delay(400);
    if (digitalRead(BUTTON) == LOW) {
      break;
    }
  }
  jpegClass.draw(&LittleFS, "/Shenron.jpeg", jpegDrawCallback, true, -2, -2, 482, 482);
  delay(10000);
  gfx->fillScreen(BK_Fill);
  draw_grid();
  Reset();
}

void Reset() {
  jpegClass.draw(&LittleFS, "/reset.jpg", jpegDrawCallback, true, -2, -2, 482, 482);
  delay(3000);
  gfx->fillScreen(BK_Fill);

  randomSeed(millis());
  for (int i=0; i<=6; i++) {
    DB_x[i] = random(-dist_range, dist_range);
    DB_y[i] = random(sqrt(pow(dist_range,2) - pow(DB_x[i],2))*-1, sqrt(pow(dist_range,2) - pow(DB_x[i],2)));
    DB_x[i] = DB_x[i]/length_x*0.00001 + location_lng;
    DB_y[i] = DB_y[i]/length_y*0.00001 + location_lat;
    collection[i] = 0;
    EEPROM.put(i*4, DB_x[i]); 
    EEPROM.put(i*4+28, DB_y[i]);
    EEPROM.put(i+56, collection[i]);
  }
  shuffle_count = max_shuffle;
  EEPROM.put(63, shuffle_count);
  EEPROM.commit();
}

void Shuffle(int i) {
  gfx->setCursor(230, 108);
  gfx->setTextColor(BK_Fill);
  gfx->print(shuffle_count);
  if ((nearest_Distance <= scope) && (shuffle_count > 0)){
    for (int n=0; n<=2; n++) {
      gfx->fillCircle(Radar_x[i], Radar_y[i], 5, RED);
      gfx->fillCircle(Radar_x[i], Radar_y[i], 3, YELLOW);
      delay(600);
      gfx->fillCircle(Radar_x[i], Radar_y[i], 5, BK_Fill);
      gfx->fillCircle(Radar_x[i], Radar_y[i], 3, YELLOW);
      delay(400);
    }
    randomSeed(millis());
    DB_x[i] = random(-dist_range, dist_range);
    DB_y[i] = random(sqrt(pow(dist_range,2) - pow(DB_x[i],2))*-1, sqrt(pow(dist_range,2) - pow(DB_x[i],2)));
    DB_x[i] = DB_x[i]/length_x*0.00001 + location_lng;
    DB_y[i] = DB_y[i]/length_y*0.00001 + location_lat;
    shuffle_count = shuffle_count - 1;
    EEPROM.put(i*4, DB_x[i]); 
    EEPROM.put(i*4+28, DB_y[i]);
    EEPROM.put(63, shuffle_count);
    EEPROM.commit();
  }
}

void ButtonRead() {
  if (digitalRead(BUTTON) == LOW) {
    Last_Button = millis();
    if (Button_State == 0) {
      Button_State = 1;
      Button_Start = millis();
    } else {
      if ((millis() - Button_Start >= 3000) && (Button_Flag == 0)) {
        if (display_mode == 0) {
          Reset();
        } else { 
          Shuffle(nearest_DB);
        }
      Button_Flag = 1;
      }
    }
  } else {
    if (Button_State == 1) {
      Button_State = 0;
      Button_Flag = 0;
      if (millis() - Button_Start < 3000) {
        if (display_mode == 0) {
          display_mode = 1;
        } else {
          display_mode = 0;
        }
        gfx->fillScreen(BK_Fill);
        draw_grid();
      }
    }
  }
}


//------------------------
//          Main
//------------------------
void setup() {
  pinMode(BUTTON, INPUT_PULLUP);
  WiFi.mode(WIFI_OFF);
  EEPROM.begin(64); //7*4 + 7*4 + 7*1 + 1
  for (int i; i<=6; i++) {
    EEPROM.get(i*4, DB_x[i]);
    EEPROM.get(i*4+28, DB_y[i]);
    EEPROM.get(i+56, collection[i]);
  }
  EEPROM.get(63, shuffle_count);
  ss.begin(GPSBaud);
  gfx->begin();
  gfx->fillScreen(BK_Fill);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  Wire.begin();
  compass.init();
  compass.enableDefault();
  compass.m_min = (LSM303::vector<int16_t>){  -566,   -625,   -567};   //Put GY-511(LSM303) caliblation result here
  compass.m_max = (LSM303::vector<int16_t>){  +445,   +505,   +566};   //Put GY-511(LSM303) caliblation result here
  LittleFS.begin();
}

void loop() {
  float location_lng = gps.location.lng();    //comment out for Demo Mode
  float location_lat = gps.location.lat();    //comment out for Demo Mode
  
  compass.read();
  float heading_tmp = compass.heading();
  if (heading > 270 && heading_tmp < 90) {
    heading_tmp = heading_tmp + 360;
  } else if (heading < 90 && heading_tmp > 270) {
    heading_tmp = heading_tmp - 360;
  }
  heading = (heading + heading_tmp)/2;
  if (heading > 360) {
    heading = heading - 360;
  } else if (heading < 0 ) {
    heading = heading  + 360;
  }
  
  int Satellites = gps.satellites.value();
  if (Satellites == 0) {
    gfx->setTextColor(BK_RED);
  }
  else if (Satellites < 3) {
    gfx->setTextColor(BK_ORANGE);
  } else {
    gfx->setTextColor(BK_GREEN);
  }
  gfx->setCursor(5, 123);
  gfx->print(Satellites);
  gfx->setTextColor(BK_GREEN);
  gfx->setCursor(230, 108);
  gfx->print(shuffle_count);

  smartDelay(300);

  writeDistance(BK_Fill);
  length_x = cos(location_lat/180*pi)*6378.137*2*pi/36000000*1000; //Distance M at 0.1 second of longitude
  
  farthest_Distance = 0;
  nearest_Distance = 12742000;
  for (int i=0; i<=6; i++) {
    if (collection[i] == 1) { continue; }
    gfx->fillCircle(Radar_x[i], Radar_y[i], 4, BK_Fill);
    Distance_x[i] = (location_lng - DB_x[i])*100000*length_x;
    Distance_y[i] = (location_lat - DB_y[i])*100000*length_y;
    Distance[i] = calcOriginHypo(Distance_x[i], Distance_y[i]);
    if (i == 0) {
      farthest_Distance = Distance[0];
      farthest_DB = 0;
      nearest_Distance = Distance[0];
      nearest_DB = 0;
      scale = Distance[0] * 1.2;
    } else {
      if (farthest_Distance < Distance[i]) {
        farthest_Distance = Distance[i];
        farthest_DB = i;
        scale = Distance[i] * 1.2;
      }
      if (nearest_Distance > Distance[i]) {
        nearest_Distance = Distance[i];
        nearest_DB = i;
      }
    }
    if (scale < dist_range) {
      scale = dist_range;
    }
  }
  
  draw_grid();
  
  for (int i=0; i<=6; i++) {
    if (collection[i] == 1) { continue; }
    if (display_mode == 0) {
      Origin_x[i] = Distance_x[i]/(scale/120)*(-1);
      Origin_y[i] = Distance_y[i]/(scale/120)*(-1);
    } else if (display_mode == 1) {
      Origin_x[i] = Distance_x[i]*(120/scope)*(-1);
      Origin_y[i] = Distance_y[i]*(120/scope)*(-1);
    }
    Origin_t[i] = calcOriginTheta(Origin_x[i], Origin_y[i]);
    Origin_h[i] = calcOriginHypo(Origin_x[i], Origin_y[i]);
    Radar_t[i] = calcTargetTheta(Origin_t[i], heading);
    Radar_x[i] = calcTargetX(Radar_t[i], Origin_h[i]) + 119;
    Radar_y[i] = 119 - calcTargetY(Radar_t[i], Origin_h[i]);
    gfx->fillCircle(Radar_x[i], Radar_y[i], 3, YELLOW);
    gfx->drawCircle(Radar_x[i], Radar_y[i], 4, BK_Fill);
  }

  if (display_mode == 1 && nearest_Distance <= get_range) {
    getDB(nearest_DB);
    collection[nearest_DB] = 1;
    EEPROM.put(nearest_DB + 56, collection[nearest_DB]);
    EEPROM.commit();
    delay(3000);
    gfx->fillScreen(BK_Fill);
    draw_grid();
  }
  
  int sum = 0;
  for (int i=0; i<=6; i++) {
    sum = sum + collection[i];
  }
  if (sum == 7) {
    Shenron();
  }
  
  writeDistance(WHITE);

  if (millis() - Last_Button >= sleep_timer) {
    WiFi.mode(WIFI_STA);
    start_light_sleep();
    while (analogRead(A0) != 1024) {
      start_light_sleep();
    }
    digitalWrite(TFT_BL, HIGH);
    Last_Button = millis();
  }
  gfx->setTextColor(BK_Fill);
  gfx->setCursor(5, 123);
  gfx->print(Satellites);
  gfx->setCursor(230, 108);
  gfx->print(shuffle_count);
    
// -----Demo Mode-----
// For demo or debug purpose, your current location is gradually updated toward the nearest drangon ball.
// Need to set Home Position manually at "Customize Settings" and disable update from GPS at top of "void loop()".

//  if ((DB_x[nearest_DB] - location_lng) > 0) {
//    location_lng = location_lng + 0.00001;
//  } else {
//    location_lng = location_lng - 0.00001;
//  }
//  if ((DB_y[nearest_DB] - location_lat) > 0) {
//    location_lat = location_lat + 0.00001;
//  } else {
//    location_lat = location_lat - 0.00001;
//  }  
}
