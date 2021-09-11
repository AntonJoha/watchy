#ifndef WATCHFACE_H
#define WATCHFACE_H
#include <Arduino.h>

class Watchface{
    public:
        //Return the state the clock is supposed to be in
        virtual int handleButtonPress(uint64_t wakeupBit, void* data);
        virtual bool drawWatchFace(void * data);
};


#endif //WATCHFACE_H