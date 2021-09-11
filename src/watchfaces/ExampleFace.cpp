#include "ExampleFace.h"
#include "Watchy.h"

int WatchfaceExample::handleButtonPress(uint64_t wakeupBit, void* data)
{
    if (wakeupBit & MENU_BTN_MASK)
    {
        return MAIN_MENU_STATE;
    }
    return WATCHFACE_STATE;
}




struct pos{
	uint16_t x;
	uint16_t y;
};

struct angle_pair{
	double x;
	double y;
};

struct vertices{
	struct pos pos[3];
};

//goes from 0 to pi/2 with increments of pi/30
double radianTable[16] = {
	0.0,
	0.104720,
	0.209440,
	0.314159,
	0.418879,
	0.523599,
	0.628319,
	0.733038,
	0.837758,
	0.942478,
	1.047198,
	1.151917,
	1.256637,
	1.361357,
	1.466077,
	1.570796
};

#define START_DEGREES 1.570796

/*

Time mapped to degrees

16 need to be mapped

double degrees

*/


#define HOUR 1
#define MINUTES 2
#define SECONDS 3
//Used to get the right angle given the specific time
//Returns a struct with an angle
//Supposed to be used like this: y = cos(t), x = sin(t) where t is the variable returned from this function
//The order is switched from the so that it starts at (0,1) and moves in the clockwise direction
//Is supposed to be given a time type
double angleWrapper(tmElements_t time, uint8_t type)
{
	uint8_t count = 0;
	if (type == HOUR)
	{
		uint8_t hour = time.Hour%12;
		uint8_t minutes = time.Minute/10;
		double amount = static_cast<double>(hour*5 + minutes);
		return radianTable[1]*(amount);
	}
	else if (type == MINUTES)
	{
		double minutes = static_cast<double>(time.Minute);
		return radianTable[1]*minutes;
	}
	else if (type == SECONDS)
	{
		uint8_t seconds = time.Second;
		while (seconds >= 15)
		{
			++count;
			seconds -= 15;
		}
		return radianTable[seconds] + count*radianTable[15];
	}
	else 
	{
		//NOT GOOD IF IT GETS HERE
		return -1;
	}

}

struct pos getPosition(uint16_t x, uint16_t y, double radian, double radius)
{
	double d_x = static_cast<double> (x);
	double d_y = static_cast<double> (y);
	struct pos toReturn;
	toReturn.x = static_cast<uint16_t>(d_x + floor(radius*cos(radian)));
	toReturn.y = static_cast<uint16_t>(d_y + floor(radius*sin(radian)));
	return toReturn;
}

struct vertices getClockVertices(double angle, double outerRadius, double innerRadius, uint16_t x = 100, uint16_t y = 100)
{
	struct vertices vertices;

	vertices.pos[0].x = x + static_cast<uint16_t>(outerRadius*sin(angle));
	vertices.pos[0].y = y - static_cast<uint16_t>(outerRadius*cos(angle));

	angle += radianTable[15];

	vertices.pos[1].x = x + static_cast<uint16_t>(innerRadius*sin(angle));
	vertices.pos[1].y = y - static_cast<uint16_t>(innerRadius*cos(angle));

	angle += 2*radianTable[15];

	vertices.pos[2].x = x + static_cast<uint16_t>(innerRadius*sin(angle));
	vertices.pos[2].y = y - static_cast<uint16_t>(innerRadius*cos(angle));

	return vertices;
}





bool WatchfaceExample::drawWatchFace(void* data)
{
    auto *display = Watchy::getDisplay();
    auto currentTime = Watchy::getTime();

	int *value = (int*) data;

	if (*value == 0) *value = 10;

	*value += 1;

	display->fillScreen(GxEPD_WHITE);
	

	double angle = angleWrapper(currentTime, MINUTES);
	struct vertices vertices = getClockVertices(angle, 70.0, 5.0);
	//display.drawPixel(p.x, p.y, GxEPD_BLACK);
	display->fillTriangle(vertices.pos[0].x,vertices.pos[0].y,vertices.pos[1].x,vertices.pos[1].y,vertices.pos[2].x,vertices.pos[2].y,GxEPD_BLACK);
	
	angle = angleWrapper(currentTime, HOUR);
	vertices = getClockVertices(angle, 50.0, 5.0);
	display->drawTriangle(vertices.pos[0].x,vertices.pos[0].y,vertices.pos[1].x,vertices.pos[1].y,vertices.pos[2].x,vertices.pos[2].y,GxEPD_BLACK);
	
	display->println(*value);

	return true;

}