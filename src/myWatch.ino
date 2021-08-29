#include "Watchy.h"
#include "ExampleFace.h"

void setup()
{
	WatchfaceExample face;
	Watchy::setWatchface(static_cast<Watchface*>(&face));
	Watchy::init();
}

void loop() {}