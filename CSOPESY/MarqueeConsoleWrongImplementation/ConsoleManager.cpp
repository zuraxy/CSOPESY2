#include "ConsoleManager.h"
#include <iostream>
#include "MainConsole.h"
#include "TypedefRepo.h"
#include "utils.h"
#include "MarqueeConsole.h"

ConsoleManager* ConsoleManager::sharedInstance = nullptr;
ConsoleManager* ConsoleManager::getInstance()
{
	return sharedInstance;
}

void ConsoleManager::initialize()
{
	sharedInstance = new ConsoleManager();
}

void ConsoleManager::destroy()
{
	delete sharedInstance;
}

ConsoleManager::~ConsoleManager()
{
}

void ConsoleManager::drawConsole() const
{
	if (this->currentConsole != nullptr)
	{
		this->currentConsole->display();
	}
	else
	{
		std::cerr << "No console assigned." << std::endl;
	}
}

void ConsoleManager::process() const
{
	if (this->currentConsole != nullptr)
	{
		this->currentConsole->process();
	}
	else
	{
		std::cerr << "No console assigned." << std::endl;
	}
}

void ConsoleManager::switchConsole(String consoleName)
{
	printMsgNewLine("Switching to console: " + consoleName);
	if (this->consoleTable.contains(consoleName))
	{
		system("cls");
		this->previousConsole = this->currentConsole;
		this->currentConsole = this->consoleTable[consoleName];
		this->currentConsole->onEnabled();
	}
	else
	{
		std::cerr << "Console not found." << std::endl;
	}
}

ConsoleManager::ConsoleManager()
{
	this->running = true;
	this->consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	const std::shared_ptr<MainConsole> mainConsole = std::make_shared<MainConsole>();
	const std::shared_ptr<MarqueeConsole> marqueeConsole = std::make_shared<MarqueeConsole>();
	this->consoleTable[MAIN_CONSOLE] = mainConsole;
	this->consoleTable[MARQUEE_CONSOLE] = marqueeConsole;

	this->switchConsole(MAIN_CONSOLE);
}

bool ConsoleManager::isScreenRegistered(const String& screenName) {
	return consoleTable.find(screenName) != consoleTable.end();
}

void ConsoleManager::returnToPreviousConsole() {
	if (previousConsole) {
		currentConsole = previousConsole;
		previousConsole = nullptr;
		system("cls");
		currentConsole->display();
	}
	else {
		std::cerr << "No previous screen to return to." << std::endl;
	}
}

bool ConsoleManager::isRunning() const
{
	return this->running;
}

std::shared_ptr<AConsole> ConsoleManager::getCurrentConsole() {
	return this->currentConsole;
}

void ConsoleManager::setCursorPosition(int posX, int posY) const
{
	COORD coord;
	coord.X = posX;
	coord.Y = posY;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}
