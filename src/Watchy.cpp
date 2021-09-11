#include "Watchy.h"

using namespace Watchy;

RTC_DATA_ATTR DS3232RTC Watchy::RTC(false);
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> Watchy::display(GxEPD2_154_D67(CS, DC, RESET, BUSY));
RTC_DATA_ATTR int menuIndex;
tmElements_t Watchy::currentTime;
RTC_DATA_ATTR int guiState;
RTC_DATA_ATTR BMA423 sensor;
RTC_DATA_ATTR bool WIFI_CONFIGURED;
RTC_DATA_ATTR bool BLE_CONFIGURED;
RTC_DATA_ATTR weatherData Watchy::currentWeather;
RTC_DATA_ATTR int weatherIntervalCounter = WEATHER_UPDATE_INTERVAL;
Watchface *Watchy::face;
RTC_DATA_ATTR byte Watchy::data[DATASIZE];

void Watchy::setWatchface(Watchface *f)
{
    Watchy::face = f;
}

byte *Watchy::getData()
{
    return Watchy::data;
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void Watchy::init(String datetime){
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause(); //get wake up reason
    Wire.begin(SDA, SCL); //init i2c

    switch (wakeup_reason)
    {
        #ifdef ESP_Watchy::RTC
        case ESP_SLEEP_WAKEUP_TIMER: //ESP Internal Watchy::RTC
            Watchy::RTC.read(Watchy::currentTime);
            Watchy::currentTime.Minute++;
            tmElements_t tm;
            tm.Month = Watchy::currentTime.Month;
            tm.Day = Watchy::currentTime.Day;
            tm.Year = Watchy::currentTime.Year;
            tm.Hour = Watchy::currentTime.Hour;
            tm.Minute = Watchy::currentTime.Minute;
            tm.Second = 0;
            time_t t = makeTime(tm);
            Watchy::RTC.set(t);
            Watchy::RTC.read(Watchy::currentTime);
            if(guiState == WATCHFACE_STATE){
                Watchy::showWatchFace(true); //partial updates on tick
            }
            else if (guiState == MAIN_MENU_STATE) Wathcy::showMenu(false);
            break;
        #endif
        case ESP_SLEEP_WAKEUP_EXT0: //Watchy::RTC Alarm
            Watchy::RTC.alarm(ALARM_2); //resets the alarm flag in the Watchy::RTC
            if(guiState == WATCHFACE_STATE){
                Watchy::RTC.read(Watchy::currentTime);
                Watchy::showWatchFace(true); //partial updates on tick
            }
            break;
        case ESP_SLEEP_WAKEUP_EXT1: //button Press
            handleButtonPress();
            break;
        default: //reset
            #ifndef ESP_Watchy::RTC
            Watchy::_rtcConfig(datetime);
            #endif
            Watchy::_bmaConfig();
            Watchy::showWatchFace(false); //full update on reset
            break;
    }
    Watchy::deepSleep();
}

void Watchy::deepSleep()
{
    #ifndef ESP_Watchy::RTC
    esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on Watchy::RTC interrupt
    #endif
    #ifdef ESP_Watchy::RTC
    esp_sleep_enable_timer_wakeup(60000000);
    #endif
    esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
    esp_deep_sleep_start();
}


void Watchy::_rtcConfig(String datetime)
{
     if(datetime != NULL){
        const time_t FUDGE(30);//fudge factor to allow for upload time, etc. (seconds, YMMV)
        tmElements_t tm;
        tm.Year = getValue(datetime, ':', 0).toInt() - YEAR_OFFSET;//offset from 1970, since year is stored in uint8_t
        tm.Month = getValue(datetime, ':', 1).toInt();
        tm.Day = getValue(datetime, ':', 2).toInt();
        tm.Hour = getValue(datetime, ':', 3).toInt();
        tm.Minute = getValue(datetime, ':', 4).toInt();
        tm.Second = getValue(datetime, ':', 5).toInt();

        time_t t = makeTime(tm) + FUDGE;
        Watchy::RTC.set(t);

    }
    //https://github.com/JChristensen/DS3232Watchy::RTC
    Watchy::RTC.squareWave(SQWAVE_NONE); //disable square wave output
    //Watchy::RTC.set(compileTime()); //set Watchy::RTC time to compile time
    Watchy::RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0); //alarm wakes up Watchy every minute
    Watchy::RTC.alarmInterrupt(ALARM_2, true); //enable alarm interrupt
    Watchy::RTC.read(Watchy::currentTime);
}


void _menuHandleButtonPress(uint64_t wakeupBit)
{
    if (wakeupBit & MENU_BTN_MASK)
    {
        switch(menuIndex)
        {
        case 0:
          Watchy::showBattery();
          break;
        case 1:
          Watchy::showBuzz();
          break;
        case 2:
          Watchy::showAccelerometer();
          break;
        case 3:
          Watchy::setTime();
          break;
        case 4:
          Watchy::setupWifi();
          break;
        case 5:
          Watchy::showUpdateFW();
          break;
        default:
          break;
        }
    }
    else if (wakeupBit & BACK_BTN_MASK)
    {
        Watchy::RTC.alarm(ALARM_2); //resets the alarm flag in the Watchy::RTC
        Watchy::RTC.read(Watchy::currentTime);
        Watchy::showWatchFace(false);
    }
    else if (wakeupBit & UP_BTN_MASK)
    {
        menuIndex--;
        if(menuIndex < 0)
        {
            menuIndex = MENU_LENGTH - 1;
        } 
        Watchy::showMenu(true);
    }
    else if (wakeupBit & DOWN_BTN_MASK)
    {
        menuIndex++;
        if(menuIndex > MENU_LENGTH - 1){
            menuIndex = 0;
        }
        Watchy::showMenu(true);
    }


//TODO LOOK OVER THIS AND MAKE IT WORK WITH WHAT I WANT TO DO
    /***************** fast menu *****************/
  bool timeout = false;
  long lastTimeout = millis();
  pinMode(MENU_BTN_PIN, INPUT);
  pinMode(BACK_BTN_PIN, INPUT);
  pinMode(UP_BTN_PIN, INPUT);
  pinMode(DOWN_BTN_PIN, INPUT);
  while(!timeout){
      if(millis() - lastTimeout > 5000){
          timeout = true;
      }else{
          if(digitalRead(MENU_BTN_PIN) == 1){
            lastTimeout = millis();
            if(guiState == MAIN_MENU_STATE){//if already in menu, then select menu item
                switch(menuIndex)
                {
                    case 0:
                    Watchy::showBattery();
                    break;
                    case 1:
                    Watchy::showBuzz();
                    break;
                    case 2:
                    Watchy::showAccelerometer();
                    break;
                    case 3:
                    Watchy::setTime();
                    break;
                    case 4:
                    Watchy::setupWifi();
                    break;
                    case 5:
                    Watchy::showUpdateFW();
                    break;
                    default:
                    break;
                }
            }else if(guiState == FW_UPDATE_STATE){
                Watchy::updateFWBegin();
            }
          }else if(digitalRead(BACK_BTN_PIN) == 1){
            lastTimeout = millis();
            if(guiState == MAIN_MENU_STATE){//exit to watch face if already in menu
            Watchy::RTC.alarm(ALARM_2); //resets the alarm flag in the Watchy::RTC
            Watchy::RTC.read(Watchy::currentTime);
            Watchy::showWatchFace(false);
            break; //leave loop
            }else if(guiState == APP_STATE){
            Watchy::showMenu(false);//exit to menu if already in app
            }else if(guiState == FW_UPDATE_STATE){
            Watchy::showMenu(false);//exit to menu if already in app
            }
          }else if(digitalRead(UP_BTN_PIN) == 1){
            lastTimeout = millis();
            if(guiState == MAIN_MENU_STATE){//increment menu index
            menuIndex--;
            if(menuIndex < 0){
                menuIndex = MENU_LENGTH - 1;
            }
            Watchy::showFastMenu();
            }
          }else if(digitalRead(DOWN_BTN_PIN) == 1){
            lastTimeout = millis();
            if(guiState == MAIN_MENU_STATE){//decrement menu index
            menuIndex++;
            if(menuIndex > MENU_LENGTH - 1){
                menuIndex = 0;
            }
            Watchy::showFastMenu();
            }
          }
      }
  }
}

void _watchFaceHandleButtonPress(uint64_t wakeupBit)
{
    guiState = face->handleButtonPress(wakeupBit, &Watchy::data);
    if(guiState = MAIN_MENU_STATE) Watchy::showMenu(true);
}

void _appHandleButtonPress(uint64_t wakeupBit)
{
    if (wakeupBit & BACK_BTN_MASK) Watchy::showMenu(false);//exit to menu if already in app
    else
    {
        //Pass input to app
    }
}

void _fwUpdateHandleButtonPress(uint64_t wakeupBit)
{
    if(wakeupBit & MENU_BTN_MASK)
    {
        Watchy::updateFWBegin();
    }
    else if (wakeupBit & BACK_BTN_MASK)
    {
        Watchy::showMenu(false);//exit to menu if already in app
    }
}

void Watchy::handleButtonPress(){
    uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();

    if (guiState == MAIN_MENU_STATE) _menuHandleButtonPress(wakeupBit);
    else if (guiState == FW_UPDATE_STATE) _fwUpdateHandleButtonPress(wakeupBit);
    else if (guiState == APP_STATE) _appHandleButtonPress(wakeupBit);
    else if (guiState == WATCHFACE_STATE) _watchFaceHandleButtonPress(wakeupBit);  
    
    if (guiState == MAIN_MENU_STATE) showMenu(false);
    display.hibernate();
}

void Watchy::showMenu(bool partialRefresh){
    display.init(0, false); //_initial_refresh to false to prevent full update on init
    display.setFullWindow();
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);

    int16_t  x1, y1;
    uint16_t w, h;
    int16_t yPos;

    const char *menuItems[] = {"Check Battery", "Vibrate Motor", "Show Accelerometer", "Set Time", "Setup WiFi", "Update Firmware"};
    for(int i=0; i<MENU_LENGTH; i++){
    yPos = 30+(MENU_HEIGHT*i);
    display.setCursor(0, yPos);
    if(i == menuIndex){
        display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);
        display.fillRect(x1-1, y1-10, 200, h+15, GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.println(menuItems[i]);
    }else{
        display.setTextColor(GxEPD_WHITE);
        display.println(menuItems[i]);
    }
    }

    display.display(partialRefresh);
    //display.hibernate();

    guiState = MAIN_MENU_STATE;
}

void Watchy::showFastMenu(){
    display.setFullWindow();
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);

    int16_t  x1, y1;
    uint16_t w, h;
    int16_t yPos;

    const char *menuItems[] = {"Check Battery", "Vibrate Motor", "Show Accelerometer", "Set Time", "Setup WiFi", "Update Firmware"};
    for(int i=0; i<MENU_LENGTH; i++){
    yPos = 30+(MENU_HEIGHT*i);
    display.setCursor(0, yPos);
    if(i == menuIndex){
        display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);
        display.fillRect(x1-1, y1-10, 200, h+15, GxEPD_WHITE);
        display.setTextColor(GxEPD_BLACK);
        display.println(menuItems[i]);
    }else{
        display.setTextColor(GxEPD_WHITE);
        display.println(menuItems[i]);
    }
    }

    display.display(true);

    guiState = MAIN_MENU_STATE;
}

void Watchy::showBattery(){
    display.init(0, false); //_initial_refresh to false to prevent full update on init
    display.setFullWindow();
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(20, 30);
    display.println("Battery Voltage:");
    float voltage = getBatteryVoltage();
    display.setCursor(70, 80);
    display.print(voltage);
    display.println("V");
    display.display(false); //full refresh
    display.hibernate();

    guiState = APP_STATE;
}

void Watchy::showBuzz(){
    display.init(0, false); //_initial_refresh to false to prevent full update on init
    display.setFullWindow();
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(70, 80);
    display.println("Buzz!");
    display.display(false); //full refresh
    display.hibernate();
    vibMotor();
    Watchy::showMenu(false);
}

void Watchy::vibMotor(uint8_t intervalMs, uint8_t length){
    pinMode(VIB_MOTOR_PIN, OUTPUT);
    bool motorOn = false;
    for(int i=0; i<length; i++){
        motorOn = !motorOn;
        digitalWrite(VIB_MOTOR_PIN, motorOn);
        delay(intervalMs);
    }
}

void Watchy::setTime(){

    guiState = APP_STATE;

    Watchy::RTC.read(Watchy::currentTime);

    int8_t minute = Watchy::currentTime.Minute;
    int8_t hour = Watchy::currentTime.Hour;
    int8_t day = Watchy::currentTime.Day;
    int8_t month = Watchy::currentTime.Month;
    int8_t year = Watchy::currentTime.Year + YEAR_OFFSET - 2000;

    int8_t setIndex = SET_HOUR;

    int8_t blink = 0;

    pinMode(DOWN_BTN_PIN, INPUT);
    pinMode(UP_BTN_PIN, INPUT);
    pinMode(MENU_BTN_PIN, INPUT);
    pinMode(BACK_BTN_PIN, INPUT);

    display.init(0, true); //_initial_refresh to false to prevent full update on init
    display.setFullWindow();

    while(1)
    {

        if(digitalRead(MENU_BTN_PIN) == 1){
            setIndex++;
            if(setIndex > SET_DAY){
                break; //LEAVE WHILE LOOP 
            }
        }
        if(digitalRead(BACK_BTN_PIN) == 1){
            if(setIndex != SET_HOUR){
                setIndex--;
            }
            else
            {
                break; //LEAVE WHILE LOOP
            }
        }

        blink = 1 - blink;

        if(digitalRead(DOWN_BTN_PIN) == 1){
            blink = 1;
            switch(setIndex){
            case SET_HOUR:
                hour == 23 ? (hour = 0) : hour++;
                break;
            case SET_MINUTE:
                minute == 59 ? (minute = 0) : minute++;
                break;
            case SET_YEAR:
                year == 99 ? (year = 20) : year++;
                break;
            case SET_MONTH:
                month == 12 ? (month = 1) : month++;
                break;
            case SET_DAY:
                day == 31 ? (day = 1) : day++;
                break;
            default:
                break;
            }
        }

        if(digitalRead(UP_BTN_PIN) == 1){
            blink = 1;
            switch(setIndex){
            case SET_HOUR:
                hour == 0 ? (hour = 23) : hour--;
                break;
            case SET_MINUTE:
                minute == 0 ? (minute = 59) : minute--;
                break;
            case SET_YEAR:
                year == 20 ? (year = 99) : year--;
                break;
            case SET_MONTH:
                month == 1 ? (month = 12) : month--;
                break;
            case SET_DAY:
                day == 1 ? (day = 31) : day--;
                break;
            default:
                break;
            }
        }

        display.fillScreen(GxEPD_BLACK);
        display.setTextColor(GxEPD_WHITE);
        display.setFont(&DSEG7_Classic_Bold_53);

        display.setCursor(5, 80);
        if(setIndex == SET_HOUR){//blink hour digits
            display.setTextColor(blink ? GxEPD_WHITE : GxEPD_BLACK);
        }
        if(hour < 10){
            display.print("0");
        }
        display.print(hour);

        display.setTextColor(GxEPD_WHITE);
        display.print(":");

        display.setCursor(108, 80);
        if(setIndex == SET_MINUTE){//blink minute digits
            display.setTextColor(blink ? GxEPD_WHITE : GxEPD_BLACK);
        }
        if(minute < 10){
            display.print("0");
        }
        display.print(minute);

        display.setTextColor(GxEPD_WHITE);

        display.setFont(&FreeMonoBold9pt7b);
        display.setCursor(45, 150);
        if(setIndex == SET_YEAR){//blink minute digits
            display.setTextColor(blink ? GxEPD_WHITE : GxEPD_BLACK);
        }
        display.print(2000+year);

        display.setTextColor(GxEPD_WHITE);
        display.print("/");

        if(setIndex == SET_MONTH){//blink minute digits
            display.setTextColor(blink ? GxEPD_WHITE : GxEPD_BLACK);
        }
        if(month < 10){
            display.print("0");
        }
        display.print(month);

        display.setTextColor(GxEPD_WHITE);
        display.print("/");

        if(setIndex == SET_DAY){//blink minute digits
            display.setTextColor(blink ? GxEPD_WHITE : GxEPD_BLACK);
        }
        if(day < 10){
            display.print("0");
        }
        display.print(day);
        display.display(true); //partial refresh
    } //END OF WHILE(1)

    display.hibernate();

    const time_t FUDGE(10);//fudge factor to allow for upload time, etc. (seconds, YMMV)
    tmElements_t tm;
    tm.Month = month;
    tm.Day = day;
    tm.Year = year + 2000 - YEAR_OFFSET;//offset from 1970, since year is stored in uint8_t
    tm.Hour = hour;
    tm.Minute = minute;
    tm.Second = 0;

    time_t t = makeTime(tm) + FUDGE;
    Watchy::RTC.set(t);

    Watchy::showMenu(false);

}

GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> *Watchy::getDisplay()
{
    return &Watchy::display;
}

tmElements_t Watchy::getTime()
{
    return Watchy::currentTime;
}

void Watchy::showAccelerometer(){
    display.init(0, true); //_initial_refresh to false to prevent full update on init
    display.setFullWindow();
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_WHITE);

    Accel acc;

    long previousMillis = 0;
    long interval = 200;

    guiState = APP_STATE;

    pinMode(BACK_BTN_PIN, INPUT);

    while(1){

    unsigned long currentMillis = millis();

    if(digitalRead(BACK_BTN_PIN) == 1){
        break;
    }

    if(currentMillis - previousMillis > interval){
        previousMillis = currentMillis;
        // Get acceleration data
        bool res = sensor.getAccel(acc);
        uint8_t direction = sensor.getDirection();
        display.fillScreen(GxEPD_BLACK);
        display.setCursor(0, 30);
        if(res == false) {
            display.println("getAccel FAIL");
        }else{
        display.print("  X:"); display.println(acc.x);
        display.print("  Y:"); display.println(acc.y);
        display.print("  Z:"); display.println(acc.z);

        display.setCursor(30, 130);
        switch(direction){
            case DIRECTION_DISP_DOWN:
                display.println("FACE DOWN");
                break;
            case DIRECTION_DISP_UP:
                display.println("FACE UP");
                break;
            case DIRECTION_BOTTOM_EDGE:
                display.println("BOTTOM EDGE");
                break;
            case DIRECTION_TOP_EDGE:
                display.println("TOP EDGE");
                break;
            case DIRECTION_RIGHT_EDGE:
                display.println("RIGHT EDGE");
                break;
            case DIRECTION_LEFT_EDGE:
                display.println("LEFT EDGE");
                break;
            default:
                display.println("ERROR!!!");
                break;
        }

        }
        display.display(true); //full refresh
    }
    }

    Watchy::showMenu(false);
}

void Watchy::showWatchFace(bool partialRefresh){
  display.init(0, false); //_initial_refresh to false to prevent full update on init
  display.setFullWindow();
  Watchy::face->drawWatchFace(&Watchy::data);
  display.display(partialRefresh); //partial refresh
  display.hibernate();
  guiState = WATCHFACE_STATE;
}

weatherData Watchy::getWeatherData(){
    if(weatherIntervalCounter >= WEATHER_UPDATE_INTERVAL){ //only update if WEATHER_UPDATE_INTERVAL has elapsed i.e. 30 minutes
        if(connectWiFi()){//Use Weather API for live data if WiFi is connected
            HTTPClient http;
            http.setConnectTimeout(3000);//3 second max timeout
            String weatherQueryURL = String(OPENWEATHERMAP_URL) + String(CITY_NAME) + String(",") + String(COUNTRY_CODE) + String("&units=") + String(TEMP_UNIT) + String("&appid=") + String(OPENWEATHERMAP_APIKEY);
            http.begin(weatherQueryURL.c_str());
            int httpResponseCode = http.GET();
            if(httpResponseCode == 200) {
                String payload = http.getString();
                JSONVar responseObject = JSON.parse(payload);
                Watchy::currentWeather.temperature = int(responseObject["main"]["temp"]);
                Watchy::currentWeather.weatherConditionCode = int(responseObject["weather"][0]["id"]);
            }else{
                //http error
            }
            http.end();
            //turn off radios
            WiFi.mode(WIFI_OFF);
            btStop();
        }else{//No WiFi, use Watchy::RTC Temperature
            uint8_t temperature = Watchy::RTC.temperature() / 4; //celsius
            if(strcmp(TEMP_UNIT, "imperial") == 0){
                temperature = temperature * 9. / 5. + 32.; //fahrenheit
            }
            Watchy::currentWeather.temperature = temperature;
            Watchy::currentWeather.weatherConditionCode = 800;
        }
        weatherIntervalCounter = 0;
    }else{
        weatherIntervalCounter++;
    }
    return Watchy::currentWeather;
}

float Watchy::getBatteryVoltage(){
    return analogRead(ADC_PIN) / 4096.0 * 7.23;
}

uint16_t Watchy::_readRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len)
{
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)address, (uint8_t)len);
    uint8_t i = 0;
    while (Wire.available()) {
        data[i++] = Wire.read();
    }
    return 0;
}

uint16_t Watchy::_writeRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len)
{
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(data, len);
    return (0 !=  Wire.endTransmission());
}

void Watchy::_bmaConfig(){

    if (sensor.begin(Watchy::_readRegister, Watchy::_writeRegister, delay) == false) {
        //fail to init BMA
        return;
    }

    // Accel parameter structure
    Acfg cfg;
    /*!
        Output data rate in Hz, Optional parameters:
            - BMA4_OUTPUT_DATA_RATE_0_78HZ
            - BMA4_OUTPUT_DATA_RATE_1_56HZ
            - BMA4_OUTPUT_DATA_RATE_3_12HZ
            - BMA4_OUTPUT_DATA_RATE_6_25HZ
            - BMA4_OUTPUT_DATA_RATE_12_5HZ
            - BMA4_OUTPUT_DATA_RATE_25HZ
            - BMA4_OUTPUT_DATA_RATE_50HZ
            - BMA4_OUTPUT_DATA_RATE_100HZ
            - BMA4_OUTPUT_DATA_RATE_200HZ
            - BMA4_OUTPUT_DATA_RATE_400HZ
            - BMA4_OUTPUT_DATA_RATE_800HZ
            - BMA4_OUTPUT_DATA_RATE_1600HZ
    */
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
    /*!
        G-range, Optional parameters:
            - BMA4_ACCEL_RANGE_2G
            - BMA4_ACCEL_RANGE_4G
            - BMA4_ACCEL_RANGE_8G
            - BMA4_ACCEL_RANGE_16G
    */
    cfg.range = BMA4_ACCEL_RANGE_2G;
    /*!
        Bandwidth parameter, determines filter configuration, Optional parameters:
            - BMA4_ACCEL_OSR4_AVG1
            - BMA4_ACCEL_OSR2_AVG2
            - BMA4_ACCEL_NORMAL_AVG4
            - BMA4_ACCEL_CIC_AVG8
            - BMA4_ACCEL_RES_AVG16
            - BMA4_ACCEL_RES_AVG32
            - BMA4_ACCEL_RES_AVG64
            - BMA4_ACCEL_RES_AVG128
    */
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

    /*! Filter performance mode , Optional parameters:
        - BMA4_CIC_AVG_MODE
        - BMA4_CONTINUOUS_MODE
    */
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;

    // Configure the BMA423 accelerometer
    sensor.setAccelConfig(cfg);

    // Enable BMA423 accelerometer
    // Warning : Need to use feature, you must first enable the accelerometer
    // Warning : Need to use feature, you must first enable the accelerometer
    sensor.enableAccel();

    struct bma4_int_pin_config config ;
    config.edge_ctrl = BMA4_LEVEL_TRIGGER;
    config.lvl = BMA4_ACTIVE_HIGH;
    config.od = BMA4_PUSH_PULL;
    config.output_en = BMA4_OUTPUT_ENABLE;
    config.input_en = BMA4_INPUT_DISABLE;
    // The correct trigger interrupt needs to be configured as needed
    sensor.setINTPinConfig(config, BMA4_INTR1_MAP);

    struct bma423_axes_remap remap_data;
    remap_data.x_axis = 1;
    remap_data.x_axis_sign = 0xFF;
    remap_data.y_axis = 0;
    remap_data.y_axis_sign = 0xFF;
    remap_data.z_axis = 2;
    remap_data.z_axis_sign = 0xFF;
    // Need to raise the wrist function, need to set the correct axis
    sensor.setRemapAxes(&remap_data);

    // Enable BMA423 isStepCounter feature
    sensor.enableFeature(BMA423_STEP_CNTR, true);
    // Enable BMA423 isTilt feature
    sensor.enableFeature(BMA423_TILT, true);
    // Enable BMA423 isDoubleClick feature
    sensor.enableFeature(BMA423_WAKEUP, true);

    // Reset steps
    sensor.resetStepCounter();

    // Turn on feature interrupt
    sensor.enableStepCountInterrupt();
    sensor.enableTiltInterrupt();
    // It corresponds to isDoubleClick interrupt
    sensor.enableWakeupInterrupt();
}

void Watchy::setupWifi(){
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  wifiManager.setTimeout(WIFI_AP_TIMEOUT);
  wifiManager.setAPCallback(_configModeCallback);
  if(!wifiManager.autoConnect(WIFI_AP_SSID)) {//WiFi setup failed
    display.init(0, false); //_initial_refresh to false to prevent full update on init
    display.setFullWindow();
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(0, 30);
    display.println("Setup failed &");
    display.println("timed out!");
    display.display(false); //full refresh
    display.hibernate();
  }else{
    display.init(0, false);//_initial_refresh to false to prevent full update on init
    display.setFullWindow();
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_WHITE);
    display.println("Connected to");
    display.println(WiFi.SSID());
    display.display(false);//full refresh
    display.hibernate();
  }
  //turn off radios
  WiFi.mode(WIFI_OFF);
  btStop();

  guiState = APP_STATE;
}

void Watchy::_configModeCallback (WiFiManager *myWiFiManager) {
  display.init(0, false); //_initial_refresh to false to prevent full update on init
  display.setFullWindow();
  display.fillScreen(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_WHITE);
  display.setCursor(0, 30);
  display.println("Connect to");
  display.print("SSID: ");
  display.println(WIFI_AP_SSID);
  display.print("IP: ");
  display.println(WiFi.softAPIP());
  display.display(false); //full refresh
  display.hibernate();
}

bool Watchy::connectWiFi(){
    if(WL_CONNECT_FAILED == WiFi.begin()){//WiFi not setup, you can also use hard coded credentials with WiFi.begin(SSID,PASS);
        WIFI_CONFIGURED = false;
    }else{
        if(WL_CONNECTED == WiFi.waitForConnectResult()){//attempt to connect for 10s
            WIFI_CONFIGURED = true;
        }else{//connection failed, time out
            WIFI_CONFIGURED = false;
            //turn off radios
            WiFi.mode(WIFI_OFF);
            btStop();
        }
    }
    return WIFI_CONFIGURED;
}

void Watchy::showUpdateFW(){
    display.init(0, false); //_initial_refresh to false to prevent full update on init
    display.setFullWindow();
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(0, 30);
    display.println("Please Visit");
    display.println("watchy.sqfmi.com");
    display.println("with a Bluetooth");
    display.println("enabled device");
    display.println(" ");
    display.println("Press menu button");
    display.println("again when ready");
    display.println(" ");
    display.println("Keep USB powered");
    display.display(false); //full refresh
    display.hibernate();

    guiState = FW_UPDATE_STATE;
}

void Watchy::updateFWBegin(){
    display.init(0, false); //_initial_refresh to false to prevent full update on init
    display.setFullWindow();
    display.fillScreen(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(GxEPD_WHITE);
    display.setCursor(0, 30);
    display.println("Bluetooth Started");
    display.println(" ");
    display.println("Watchy BLE OTA");
    display.println(" ");
    display.println("Waiting for");
    display.println("connection...");
    display.display(false); //full refresh

    BLE BT;
    BT.begin("Watchy BLE OTA");
    int prevStatus = -1;
    int currentStatus;

    while(1){
    currentStatus = BT.updateStatus();
    if(prevStatus != currentStatus || prevStatus == 1){
        if(currentStatus == 0){
        display.setFullWindow();
        display.fillScreen(GxEPD_BLACK);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(GxEPD_WHITE);
        display.setCursor(0, 30);
        display.println("BLE Connected!");
        display.println(" ");
        display.println("Waiting for");
        display.println("upload...");
        display.display(false); //full refresh
        }
        if(currentStatus == 1){
        display.setFullWindow();
        display.fillScreen(GxEPD_BLACK);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(GxEPD_WHITE);
        display.setCursor(0, 30);
        display.println("Downloading");
        display.println("firmware:");
        display.println(" ");
        display.print(BT.howManyBytes());
        display.println(" bytes");
        display.display(true); //partial refresh
        }
        if(currentStatus == 2){
        display.setFullWindow();
        display.fillScreen(GxEPD_BLACK);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(GxEPD_WHITE);
        display.setCursor(0, 30);
        display.println("Download");
        display.println("completed!");
        display.println(" ");
        display.println("Rebooting...");
        display.display(false); //full refresh

        delay(2000);
        esp_restart();
        }
        if(currentStatus == 4){
        display.setFullWindow();
        display.fillScreen(GxEPD_BLACK);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(GxEPD_WHITE);
        display.setCursor(0, 30);
        display.println("BLE Disconnected!");
        display.println(" ");
        display.println("exiting...");
        display.display(false); //full refresh
        delay(1000);
        break;
        }
        prevStatus = currentStatus;
    }
    delay(100);
    }

    //turn off radios
    WiFi.mode(WIFI_OFF);
    btStop();
    Watchy::showMenu(false);
}