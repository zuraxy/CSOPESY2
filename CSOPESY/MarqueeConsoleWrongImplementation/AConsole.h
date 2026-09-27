#pragma once
#include <string>
class AConsole
{
public:
	typedef std::string String;
	AConsole(String name);
	~AConsole() = default;

	String getName();
	virtual void onEnabled() = 0; // Called when the console is first drawn
	virtual void display() = 0; // called per frame, drawing proper
	virtual void process() = 0; // process input

	String name;
	friend class ConsoleManager;
};