#include "AConsole.h"
#include "TypedefRepo.h"

AConsole::AConsole(String name)
{
	this->name = name;
}

String AConsole::getName()
{
	return this->name;
}


