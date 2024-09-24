/*
// MAX7219_64*8_Clock.ino
// MAX7219 64*8 LED display
// Version 1.0
// Modified by YadaGuo
// Date: 2024/9/24
功能说明：
1. EPS8266连接WIFI，连接成功后显示Online字样，连接失败显示Offline字样。
2. 连接NTP服务器对时，获取当前时间，显示时间,日期,星期。
3. OTA更新。。
4. 时间，日期，星期，文字 轮流显示。
*/


#include <ESP8266WiFi.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <time.h>
#include "Font_Data.h"
#include <ArduinoOTA.h>

//*********MAX7219**********
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 8  //8个MAX7219模块
#define CLK_PIN   14   //D5
#define CS_PIN    15   //D8
#define DATA_PIN  13   //D7
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
#define SPEED_TIME 80 // 数字越小越快,0最快
#define PAUSE_TIME 5000


//********** WiFi Setting ******************
const char *ssid = "xxxxxx";           // 改成自家WIFI SSID
const char *password = "xxxxxxxxx"; // 密码
const char *NTPServer = "cn.pool.ntp.org";
const int gmtOffset_sec = 28800; // 8*3600


//********** 全局变量 **************
char Buff[60];
char szTime[9];
uint8_t h, m, s, month, day, week;
int year;
const char *Week[7] = {"Sunday", "Monday", "Tuesday", "Wed.", "Thursday", "Friday", "Saturday"};

void getTime(char *szTime, bool f=true)
{
 time_t now = time(nullptr);            // 获取当前时间戳
 struct tm *timeinfo = localtime(&now); // 转为本地时间
  h = timeinfo->tm_hour;
  m = timeinfo->tm_min;
  s = timeinfo->tm_sec;
  year = timeinfo->tm_year + 1900;
  month = timeinfo->tm_mon + 1;
  day = timeinfo->tm_mday;
  week = timeinfo->tm_wday;
  // 将时间转换为字符串: srtftime(存储产生的结果,最大size,tm中对应的值)
  //strftime(szTime, 9, "%R", timeinfo); // R = HH:MM, X(or T) = HH:MM:SS
   sprintf(szTime, "%02d%c%02d", h,(f ? ':' : ' '), m);  
   //strftime(szData,15, "%F", timeinfo);  // yyyy-mm-dd
   //strftime(szWeek,10, "%A", timeinfo);  //A-英文全称  a-缩写
}


void getData(char *Buff)
{
  sprintf(Buff, "%02d-%02d",  month, day);  // yyyy-mm-dd
}

void getWeek(char *Buff)
{
  sprintf(Buff, "%s", Week[week]);
}

void getNTP() // 对时服务器
{
  // WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  delay(500);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    while (!P.displayAnimate());
    P.displayZoneText(0, "Online", PA_CENTER, SPEED_TIME, 2000, PA_PRINT, PA_NO_EFFECT);
  }
  Serial.println("WiFi Connected");
  configTime(28800, 0, NTPServer);
  while (!time(nullptr))
  {
    delay(1000);
  }
  Serial.print("NTP Time Update");
}

void setup(void)
{
  P.begin(2);
  P.setZone(0, 0, 4);
  P.setZone(1, 5, 7);
  P.setFont(1, GF4x7p);
  //P.setFont(0, PHP5X7);  //字体有问题，有的字母显示不出来，数字OK

  P.setInvert(false);
  P.displayZoneText(1, szTime, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_PRINT, PA_NO_EFFECT);

  getNTP();
  getTime(szTime);

    // OTA设置并启动
  ArduinoOTA.setHostname("max7219_64x8_time");  //Arduino IDE--工具--端口 选择max7219_64x8_time, 不连接数据线也可更新代码
  ArduinoOTA.setPassword("12345678");
  ArduinoOTA.begin();
}

void loop(void)
{
  ArduinoOTA.handle();
  static uint32_t lastTime = 0; 
  static bool flasher = false; 
  static uint8_t  display = 0;  // current display mode
  P.displayAnimate();
  if (P.getZoneStatus(0))
  {
    switch (display)
    {
      case 0: // 日期
         //P.setTextEffect(0, PA_SCROLL_UP, PA_SCROLL_UP);
        P.displayZoneText(0, Buff, PA_CENTER, 10, PAUSE_TIME, PA_RANDOM, PA_RANDOM);
        getData(Buff);
        
        display++;
        break;

      case 1: // 星期
        //P.setTextEffect(0, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
        P.displayZoneText(0, Buff, PA_CENTER, SPEED_TIME, PAUSE_TIME, PA_SCROLL_UP, PA_SCROLL_UP);
        getWeek(Buff);
        display ++;
        break;

      case 2:  //文字
        P.displayZoneText(0, "Dream", PA_CENTER, 30, PAUSE_TIME, PA_SCROLL_LEFT, PA_WIPE_CURSOR);
        display = 0;
        break;
    }
      P.displayReset(0);
  }

    // Finally, adjust the time string if we have to
  if (millis() - lastTime >= 1000)
  {
    lastTime = millis();
    getTime(szTime, flasher);
    flasher = !flasher;
    P.displayReset(1);
  }

      // 晚上23点到8点，亮度设置为2，其他时间亮度设置为3
  if (h >= 23 && h < 8 )
  {
    P.setIntensity(2);
  }
  else
  {
    P.setIntensity(3);
  }
}

