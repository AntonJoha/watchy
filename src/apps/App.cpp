#include "App.h"
#include "../config.h"

int AppFrame::start(void* info)
{
    return APP_STATE;
}

int AppFrame::handleButtonPress(uint64_t wakeupBit, void* info)
{
    return (wakeupBit & BACK_BTN_PIN) ? MAIN_MENU_STATE : APP_STATE;
}