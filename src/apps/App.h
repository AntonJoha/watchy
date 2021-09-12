#ifndef APP_H
#define APP_H
#include <Arduino.h>


class AppFrame{
    //Can be given any data needed
    public:
    int start(void* info = nullptr);
    virtual void draw(void * info);
    int handleButtonPress(uint64_t wakeupBit, void* info = nullptr);
};

#endif