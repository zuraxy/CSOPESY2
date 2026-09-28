#ifndef COMMAND_INTERPRETER_H
#define COMMAND_INTERPRETER_H

#include "AppState.h"

std::string toLower(std::string text);
std::string trim(const std::string& text);
std::vector<std::string> helpText();
void submitCommand(AppState& app);
void pollInput(AppState& app);

#endif
