#ifndef APP_H
#define APP_H


class AppFrame{
    //Can be given any data needed
    virtual int start(void* info = nullptr);
    virtual void draw();
    virtual int handleButtonPress(uint64_t wakeupBit);
}

#endif