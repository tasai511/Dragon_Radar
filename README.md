# Dragon_Radar
<img src="https://user-images.githubusercontent.com/20789521/164951653-b47f373e-c136-4681-bdfb-4e81c3104c49.png" alt="Picture" title="Picture" width="430" height="200">

## How it works.
- By pressing and holding the button, 7 (virtual) Dragon Balls will be scattered to "a random latitude and longitude within a radius of 480 meters from the current location".
- The Dragon Balls are not real, but only visible on the Dragon Radar.
- The location of the Dragon Ball is constantly updated on the radar screen, centered on the current location.
- Pressing the button toggles between "Zoomed-In" and "Zoomed-Out" mode.
- In the Zoomed-Out mode, the distance from the current location to the farthest Dragon Ball is displayed, and the scale is automatically adjusted accordingly.
- In Zoomed-In mode, the scale is fixed at a 40m radius, the nearest Dragon Ball is locked and the distance to it is displayed.
- The distance and direction are used to find the Dragon Ball.
- When you get close to the locked Dragon Ball within a 5m radius, you can get the Dragon Ball.
- When you collect 7 Dragon Balls, a dragon appears on the screen.

Dragon Balls are not always found on the side of the road, but sometimes they are in someone's house or the middle of a field, and you cannot get close to them.
In such cases, "shuffle" is used.

- If you press and hold the button while in the "Zoomed-In" mode, the locked Dragon Ball will fly to a different location. (Can be used up to 7 times).

If there is a Dragon Ball that cannot be got even after using up the shuffle, the last resort is "Reset".

- If you press and hold the button while in "Zoomed-Out" mode, all the Dragon Balls, including those you have already collected, will fly somewhere else.

In other words, the game is over and you have to start over.

You will have to shuffle and reset quite often, so it is very rare to get to the dragon.
In addition, shuffling and resetting will send you to "a radius of 480 meters around the place where you did it," so if you repeat it, you may find yourself further and further away from home.

In this way, the game is designed to provide a challenging and mysterious adventure experience.

## Parts List
All required components are commercially available modules, so there is no need for surface mounting or other complicated work.

- **ESP8266 WeMos D1 Mini Module**
https://a.aliexpress.com/_mqn7YlC
- **Battery Shield V1.2.0 For WEMOS D1 mini**
https://a.aliexpress.com/_mP9hQFM
- **GC9A01 1.28 Inch Round LCD Module**
https://a.aliexpress.com/_mONJ6N0
- **NEO-6M GPS Module with G165 antenna**
https://a.aliexpress.com/_mqn7YlC
- **GY-511 LSM303DLHC 3-axis Compass Acceleration Sensor**
https://ja.aliexpress.com/i/32700849891.html
- **LiPo Battery 300mAh**
https://www.sengoku.co.jp/mod/sgk_cart/detail.php?code=EEHD-5GHG
- **Small push button switch**
https://akizukidenshi.com/catalog/g/gP-04368/

## Case & PCB
You can use .stl to 3D print the case and "PCB v41_2022-03-20.zip" to order a custom PCB. (ex. Fusion PCB)

![Picture2](https://user-images.githubusercontent.com/20789521/164953564-5a00600a-5fbc-4e1b-80e4-5dfe513fb888.png)
![Picture3](https://user-images.githubusercontent.com/20789521/164953706-4251f759-076a-4513-b543-43b20b9915b6.png)

## Assembly
Stack modules as follows.

<img src="https://user-images.githubusercontent.com/20789521/164956945-5437594a-7661-485d-b8d0-df116ebcbd65.png" alt="Diagram" title="Circuit Diagram" width="600">

## Circuit Diagram
<img src="https://user-images.githubusercontent.com/20789521/164956541-ea336510-6122-4e24-b0d0-81ad30a37c85.png" alt="Diagram" title="Circuit Diagram" width="300">

## Writing Code
- **[Important]** Need to disconnect NEO-6M before starting uploading from Arduino IDE since it's connected to TXD/RXD pin conflicting with USB Serial.
- That's why NEO-6M should not be directly soldered on custom PCB. (use pin socket)
- It uses LittleFS and needs to upload all images under "data" directry before uploading code. Follow the instruction at https://randomnerdtutorials.com/install-esp8266-nodemcu-littlefs-arduino/.
- You need to calibrate your GY-511 by running "Calibrate" under the "LSM303" example that will be installed with the Library, and put the result to the following location.
```
  compass.m_min = (LSM303::vector<int16_t>){  -566,   -625,   -567};   //Put GY-511(LSM303) caliblation result here
  compass.m_max = (LSM303::vector<int16_t>){  +445,   +505,   +566};   //Put GY-511(LSM303) caliblation result here
```

## Customize
- You can change "Customize Settings" on the code to customize the color and difficulty of the game (ex. max number of "Shuffle").

```
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
```

# Enjoy!!
please feel free to add comments if you have any questions. (Japanese & English)
