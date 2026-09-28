#ifndef CONSOLE_UI_H
#define CONSOLE_UI_H

#include "AppState.h"

void getConsoleSize(int& width, int& height);
void setCursorPosition(int x, int y);
void setCursorVisible(bool visible);
bool isInteractiveConsole();
void putStr(std::vector<std::string>& rows, int row, int column,
            const std::string& text);
std::string buildFrame(const AppState& app);
void drawFrame(const std::string& frame);

#endif
