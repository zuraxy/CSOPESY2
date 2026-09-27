#pragma once
#include <memory>
#include <vector>
#include "AConsole.h"
#include <unordered_map>
#include <Windows.h>
#include "TypedefRepo.h"

const String MAIN_CONSOLE = "MAIN_CONSOLE";
const String MARQUEE_CONSOLE = "MARQUEE_CONSOLE";

class ConsoleManager
{
public:
	typedef std::unordered_map<String, std::shared_ptr<AConsole>> ConsoleTable;

	static ConsoleManager* getInstance();
	static void initialize();
	static void destroy();

	void drawConsole() const;
	void process() const;
	void switchConsole(String consoleName);

	void returnToPreviousConsole();
	bool isRunning() const;

	HANDLE getConsoleHandle() const;

	bool isScreenRegistered(const String& screenName);
	std::shared_ptr<AConsole> getCurrentConsole();
	void setCursorPosition(int posX, int posY) const;

private:
	ConsoleManager();
	~ConsoleManager();
	ConsoleManager(ConsoleManager const&) {};
	ConsoleManager& operator=(ConsoleManager const&) {};
	static ConsoleManager* sharedInstance;

	ConsoleTable consoleTable;
	std::shared_ptr<AConsole> currentConsole;
	std::shared_ptr<AConsole> previousConsole;

	HANDLE consoleHandle;
	bool running = true;
};