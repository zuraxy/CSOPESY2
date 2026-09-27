#include "MarqueeConsole.h"
#include <iostream>
#include "ConsoleManager.h"
#include <conio.h>
#include <sstream>
#include <chrono>
#include <thread>
#include "utils.h"

const int WIDTH = 120;
const int HEIGHT = 20;

bool useThreads = false; 

MarqueeConsole::MarqueeConsole()
    : AConsole("MARQUEE_CONSOLE"), xPos(0), yPos(2), stopThread(false)
{
}

MarqueeConsole::~MarqueeConsole()
{
    stopThread = true; 

    if (animationThread.joinable()) {
        animationThread.join();
    }
    if (inputThread.joinable()) {
        inputThread.join();
    }
}

void MarqueeConsole::onEnabled()
{
    stopThread = false; 

    if (useThreads) {
        animationThread = std::thread(&MarqueeConsole::animate, this);
        inputThread = std::thread(&MarqueeConsole::handleInput, this);
    }
    else {
        int dx = 1;  
        int dy = 1;
        const std::string marqueeText = "This is a Marquee Console";
        const int maxXPos = WIDTH - marqueeText.length();  
        const int maxYPos = HEIGHT - 2; 

        xPos = 0;  
        yPos = 4;  

        while (!stopThread) {
            xPos += dx;
            yPos += dy;

            if (xPos <= 0 || xPos >= maxXPos) {
                dx = -dx; 
            }

            if (yPos <= 4 || yPos >= maxYPos) {
                dy = -dy; 
            }

            system("cls");  
            ConsoleManager::getInstance()->setCursorPosition(0, 0);
            std::cout << "*****************************************" << std::endl;
            std::cout << "*     Displaying a marquee console!     *" << std::endl;
            std::cout << "*****************************************" << std::endl;

            ConsoleManager::getInstance()->setCursorPosition(xPos, yPos);
            std::cout << marqueeText;

            ConsoleManager::getInstance()->setCursorPosition(0, HEIGHT - 1);
            std::cout << "Command:\\>" << currentCommand;

            ConsoleManager::getInstance()->setCursorPosition(0, HEIGHT);
            std::cout << outputBuffer.str();

            process();

            std::this_thread::sleep_for(std::chrono::milliseconds(REFRESH_DELAY));
        }
    }
}

void MarqueeConsole::animate()
{
    const std::string marqueeText = "This is a Marquee Console";
    const int maxXPos = WIDTH - marqueeText.length(); 
    const int maxYPos = HEIGHT - 2; 
    int xDir = 1;  
    int yDir = 1; 

    while (!stopThread) {
        xPos += xDir;
        yPos += yDir;

        if (xPos <= 0) {
            xPos = 0;   
            xDir = 1;   
        }
        else if (xPos >= maxXPos) {
            xPos = maxXPos;  
            xDir = -1;  
        }

        if (yPos <= 4) {
            yPos = 4;  
            yDir = 1; 
        }
        else if (yPos >= maxYPos) {
            yPos = maxYPos;
            yDir = -1; 
        }

        display();

        std::this_thread::sleep_for(std::chrono::milliseconds(REFRESH_DELAY));
    }
}

void MarqueeConsole::handleInput()
{
    while (!stopThread) {
        process();
        std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_DELAY));
    }
}

void MarqueeConsole::display()
{
    system("cls");

    ConsoleManager::getInstance()->setCursorPosition(0, 0);
    std::cout << "*****************************************" << std::endl;
    std::cout << "*     Displaying a marquee console!     *" << std::endl;
    std::cout << "*****************************************" << std::endl;

    ConsoleManager::getInstance()->setCursorPosition(xPos, yPos);
    std::cout << "This is a Marquee Console";

    ConsoleManager::getInstance()->setCursorPosition(0, HEIGHT - 1);
    std::cout << "Command:\\>" << currentCommand;

    ConsoleManager::getInstance()->setCursorPosition(0, HEIGHT);
    std::cout << outputBuffer.str();
}

void MarqueeConsole::process()
{
    if (_kbhit()) {
        char ch = _getch();

        if (ch == '\b' && !currentCommand.empty()) {
            currentCommand.pop_back();
        }
        else if (ch == '\n' || ch == '\r') {
            processCommand(); 
        }
        else {
            currentCommand.push_back(ch);
        }

        display();
    }
}

void MarqueeConsole::processCommand()
{
    if (currentCommand == "exit") {
        stopThread = true;
        system("cls");
        ConsoleManager::getInstance()->returnToPreviousConsole();
    }
    else if (currentCommand == "clear") {
        outputBuffer.str("");
    }
    else {
        outputBuffer << "Command entered: " << currentCommand << std::endl;
    }

    currentCommand.clear();
}