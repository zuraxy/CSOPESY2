#include "MainConsole.h"
#include "TypedefRepo.h"
#include "ConsoleManager.h"
#include "Process.h"
#include "AConsole.h"
#include <iostream>
#include "Utils.h"
#include <chrono>
#include <thread>
#include <sstream>
#include <memory>

extern ConsoleManager consoleManager;

MainConsole::MainConsole() : AConsole("MAIN_CONSOLE")
{
}

void MainConsole::display() {
    displayHeader();
    process();
}

void MainConsole::onEnabled()
{
    displayHeader();
}

void MainConsole::process() {
    String command;
    while (ConsoleManager::getInstance()->getCurrentConsole().get() == this) {
        std::cout << "root:\\> ";
        std::getline(std::cin, command);
        handleCommand(command);
        std::cout << std::endl;
    }
}

void MainConsole::displayHeader() {
    printHeader("3D_CSOPESY.txt");
}

std::pair<String, String> parseScreenCommand(String userInput) {
    String command;
    String name;
    std::stringstream ss(userInput);
    ss >> command;  // Gets 'screen'
    if (ss >> command && (command == "-r" || command == "-s")) {
        ss >> name;  // Gets the <name>
    }
    return { command, name };
}

void MainConsole::handleCommand(String command)
{
    String formattedInput = toLowerCase(command);
    auto consoleManager = ConsoleManager::getInstance();
    if (command == "exit")
    {
        std::cout << "Exiting the program..." << std::endl;
        exit(0);
        std::terminate();

    }
    else if (command == "initialize")
    {
        printMsg("initialize command recognized. Doing something");
    }

    else if (command == "clear" || command == "cls")
    {
        system("cls");
        displayHeader();
    }

    else if (command.substr(0, 6) == "screen")
    {
        printMsg("Doing something.");
    }

    else if (command == "scheduler-test")
    {
        printMsg("scheduler-test command recognized. Doing something");
    }
    else if (command == "scheduler-stop")
    {
        printMsg("scheduler-stop command recognized. Doing something");
    }
    else if (command == "report-util")
    {
        printMsg("report-util command recognized. Doing something");
    }
    else if (command == "marquee")
    {
        consoleManager->switchConsole("MARQUEE_CONSOLE");
    }
    else
    {
        std::cout << "Invalid command. Please try again." << std::endl;
    }
}