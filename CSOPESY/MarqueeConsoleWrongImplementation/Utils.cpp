#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

int printAscii(std::string fileName) {
    std::ifstream file;
    file.open(fileName);

    if (!file) {
        std::cerr << "Unable to open file art.txt" << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << std::endl;
    }

    file.close();
    return 0;
}

void printMsgNewLine(std::string msg) {
    std::cout << msg << std::endl;
}

void printMsg(std::string msg) {
    std::cout << msg;
}

void printHeader(std::string headerFile) {
    printAscii(headerFile);
    printMsgNewLine("Hello, Welcome to CSOPESY commandline!");
    printMsgNewLine("Type 'exit' to quit, 'clear' to clear the screen");
}

std::string toLowerCase(std::string str) {
    for (char& c : str) {
        c = std::tolower(c);
    }
    return str;
}
