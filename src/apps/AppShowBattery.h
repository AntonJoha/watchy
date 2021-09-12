#ifndef CHECK_BATTERY_H
#define CHECK_BATTERY_H
#include "App.h"


class ShowBattery : public AppFrame{
    public:
        int start(void *info = nullptr);
        void draw(void * info = nullptr);
        int handleButtonPress(uint64_t wakeupBit, void *info = nullptr);
};



#endif  //CHECK_BATTERY_H