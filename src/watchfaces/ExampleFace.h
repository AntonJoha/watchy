#ifndef WATCHFACE_EXAMPLE_H
#define WATCHFACE_EXAMPLE_H
#include "Watchface.h"


class WatchfaceExample : public Watchface
{
    public:
        int handleButtonPress(uint64_t wakeupBit, void*);
        bool drawWatchFace(void*);
};

#endif //WATCHFACE_EXAMPLE_H