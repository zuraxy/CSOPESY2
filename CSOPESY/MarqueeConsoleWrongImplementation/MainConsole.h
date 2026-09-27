#pragma once
#include "TypedefRepo.h"
#include "AConsole.h"

class MainConsole : public AConsole
{
public:
	MainConsole();
	~MainConsole() = default;

	void onEnabled();
	void display();
	void process();

private:
	void displayHeader();
	void handleCommand(String command);
};