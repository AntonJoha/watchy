#ifndef MAIN_MENU_H
#define MAIN_MENU_H
#include <Arduino>

namespace Menu{
    extern RTC_DATA_ATTR uint8_t position;
    void draw();
    int handleButtonPress();



}



#endif