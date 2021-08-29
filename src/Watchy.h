#ifndef NEWWATCH_H
#define NEWWATCH_H

#include <Arduino.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <DS3232RTC.h>
#include <GxEPD2_BW.h>
#include <Wire.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "DSEG7_Classic_Bold_53.h"
#include "BLE.h"
#include "bma.h"
#include "config.h"    
#include "Watchface.h"
#include "ExampleFace.h"

typedef struct weatherData{
    int8_t temperature;
    int8_t weatherConditionCode;
} weatherData;

namespace Watchy
{
    extern DS3232RTC RTC;
    extern GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display;
    extern tmElements_t currentTime;
    extern weatherData currentWeather;
    extern Watchface* face;
    extern byte data[2048];
    void init(String datetime = "");
    void deepSleep();
    float getBatteryVoltage();
    void vibMotor(uint8_t intervalMs = 100, uint8_t length = 20);
    GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> *getDisplay();
    tmElements_t getTime();
    void handleButtonPress();
    void showMenu(bool partialRefresh);
    void showFastMenu();
    void showBattery();
    void showBuzz();
    void showAccelerometer();
    void showUpdateFW();
    void setTime();
    void setupWifi();
    bool connectWiFi();
    weatherData getWeatherData();
    void updateFWBegin();
    byte* getData();
    void showWatchFace(bool partialRefresh);
    void drawWatchFace(); //override this method for different watch faces
    //Menu handling

    void setWatchface(Watchface*);

    void _configModeCallback(WiFiManager *myWiFiManager);
    uint16_t _readRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len);
    uint16_t _writeRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len);
    void _rtcConfig(String datetime);
    void _bmaConfig();
};


extern RTC_DATA_ATTR int guiState;
extern RTC_DATA_ATTR BMA423 sensor;
extern RTC_DATA_ATTR bool WIFI_CONFIGURED;
extern RTC_DATA_ATTR bool BLE_CONFIGURED;

#endif