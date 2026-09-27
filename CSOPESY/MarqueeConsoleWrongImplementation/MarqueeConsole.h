#pragma once
#include <sstream>
#include "AConsole.h"
#include <thread>

class MarqueeConsole : public AConsole
{
public:
	MarqueeConsole();
	~MarqueeConsole();

	void onEnabled() override;
	void display() override;
	void process() override;
	void processCommand();

	void animate();  
	void handleInput(); 

private:
	const int REFRESH_DELAY = 500; //change this value to slow/increase refresh rate of marquee
	const int POLLING_DELAY = 1; //change this value to slow/increase polling rate

	String currentCommand; 
	std::stringstream outputBuffer; 
	bool stopThread;  

	int xPos, yPos;  

	// Thread handling
	std::thread animationThread;  
	std::thread inputThread;     
};

