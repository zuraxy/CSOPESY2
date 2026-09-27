#include "ConsoleManager.h"
#include <iostream>
#include "MainConsole.h"
#include "TypedefRepo.h"

int main()
{
	ConsoleManager::initialize();

	bool running = true;
	while (running)
	{
		ConsoleManager::getInstance()->process();
		ConsoleManager::getInstance()->drawConsole();
		running = ConsoleManager::getInstance()->isRunning();
	}

	ConsoleManager::destroy();
	return 0;
}