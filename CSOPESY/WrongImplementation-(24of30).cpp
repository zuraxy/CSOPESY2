#include <iostream>
#include <conio.h>
#include <thread>

#include "display/DisplayHandler.h"

#define REFRESH_DELAY 3
#define POLLING_DELAY 1

void processMarquee();
void processPolling();

int main()
{
    // ----- SINGLE-THREADING -----
    while (DisplayHandler::getInstance().getIsRunning()) {
        processMarquee();
        processPolling();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // ----- MULTI-THREADING -----
    // std::thread marquee([]() {
    //     while (DisplayHandler::getInstance().getIsRunning()) {
    //         processMarquee();
    //         std::this_thread::sleep_for(std::chrono::milliseconds(1));
    //     }
    // });
    //
    // std::thread polling([]() {
    //     while (DisplayHandler::getInstance().getIsRunning()) {
    //         processPolling();
    //         std::this_thread::sleep_for(std::chrono::milliseconds(1));
    //     }
    // });
    //
    // polling.join();
    // marquee.join();

    return EXIT_SUCCESS;
}

void processMarquee() {
    if (DisplayHandler::getInstance().getRefreshCounter() % REFRESH_DELAY == 0) {
        DisplayHandler::clearScreen();
        DisplayHandler::getInstance().displayHeader();
        DisplayHandler::getInstance().displayMarquee();
        DisplayHandler::getInstance().displayPreviousInputs();

        std::cout << DisplayHandler::getInstance().getCommandPrefix() << " " << DisplayHandler::getInstance().currentInput;

        DisplayHandler::getInstance().resetRefreshCounter();
    }

    DisplayHandler::getInstance().incrementRefreshCounter();
}

void processPolling() {
    if (DisplayHandler::getInstance().getPollingCounter() % POLLING_DELAY == 0) {
        if (_kbhit()) {
            if (const char c = _getch(); c == '\r') {
                if (DisplayHandler::getInstance().currentInput == "exit") DisplayHandler::getInstance().exit();

                DisplayHandler::getInstance().addInput(DisplayHandler::getInstance().currentInput);
                DisplayHandler::getInstance().currentInput.clear();
            } else if (c == '\b') {
                if (!DisplayHandler::getInstance().currentInput.empty()) {
                    DisplayHandler::getInstance().currentInput.pop_back();
                }
            } else {
                DisplayHandler::getInstance().currentInput += c;
            }
        }

        DisplayHandler::getInstance().resetPollingCounter();
    }

    DisplayHandler::getInstance().incrementPollingCounter();
}

