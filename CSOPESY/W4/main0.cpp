// CSOPESY S09 Week 4 CLI Exercise - Bon Aquino 

#include <iostream>
#include <windows.h>
#include <iomanip>
#include <string>
#include <vector>
#include <ctime>
using namespace std;

struct Process {
    int gpu;
    string gpuInstance;     
    string computeInstance;  
    int pid;
    string processType;
    string processName;
    string gpuMemoryUsage;
};

void setCursorPosition(int x, int y) { //set cursor to somewhere in console
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(output, pos);
}

string truncateProcessName(const string& processName, size_t maxLength) { // truncate if exceeding certain length
    if (processName.length() > maxLength) {
        return "..." + processName.substr(processName.length() - maxLength, maxLength);
    }
    return processName;  
}
void consoleTextColor(int color) { // set console TEXT color
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(output, color);
}

void printCurrDateTime() { // get + print currDate
    tm localTime;
    time_t now = time(nullptr);

    localtime_s(&localTime, &now);

    char buffer[80];
    strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", &localTime);

    consoleTextColor(11);  
    setCursorPosition(0, 1); 
    cout << buffer << endl;
}

void printDisplay() { //p much brute force printing except the diff colors
    consoleTextColor(11); 
    cout << "+------------------------------------------------------------------------------------------+" << endl;
    cout << "| NVIDIA-SMI 551.86                 Driver Version: 551.86          CUDA Version: 12.4     |" << endl;
    cout << "|-----------------------------------------+-------------------------+----------------------+" << endl;
    cout << "| GPU  Name                     TCC/WDDM  | Bus-Id           Disp.A | Volatile Uncorr. ECC |" << endl;
    cout << "| Fan  Temp   Perf          Pwr:Usage/Cap |            Memory-Usage | GPU-Util  Compute M. |" << endl;
    cout << "|                                         |                         |               MIG M. |" << endl;
    cout << "|=========================================+=========================+======================|" << endl;
    consoleTextColor(14);  
    cout << "|   0  NVIDIA GeForce GTX 1080     WDDM   |    00000000:26:00.0  On |                  N/A |" << endl;
    cout << "| 28%   37C    P8             11W / 180W  |     701MiB /    8192MiB |      0%      Default |" << endl;
    cout << "|                                         |                         |                  N/A |" << endl;
    consoleTextColor(11); 
    cout << "+-----------------------------------------+-------------------------+----------------------+" << endl;
	cout << "" << endl;    
}

void displayProcesses(const vector<Process>& processes) { //show dummy processes
    consoleTextColor(10); 
    cout << "+------------------------------------------------------------------------------------------+" << endl;
    cout << "| Processes:                                                                               |" << endl;
    cout << "|  GPU   GI   CI        PID   Type   Process name                               GPU Memory |" << endl;
    cout << "|        ID   ID                                                                Usage      |" << endl;
    cout << "|==========================================================================================+" << endl;

    consoleTextColor(15);  
    for (const auto& process : processes) {
        string truncatedName = truncateProcessName(process.processName, 36);

        cout << "|    0   " << process.gpuInstance << "  " << process.computeInstance
            << "      " << setw(4) << process.pid
            << "    " << process.processType
            << "   " << left << setw(45) << truncatedName
            << process.gpuMemoryUsage << "      |" << endl;
    }

    consoleTextColor(10);  
    cout << "+------------------------------------------------------------------------------------------+" << endl;
}

int main() {
    vector<Process> processes = { //dummy processes
        {0, "N/A", "N/A", 1368, "C+G", "C:\\Windows\\System32\\VeryImportantFile.exe", "N/A"},
        {0, "N/A", "N/A", 2116, "C+G", "D:\\Drive\\Spyware.exe", "N/A"},
        {0, "N/A", "N/A", 4224, "C+G", "E:\\Downloads\\Malware\\Free64gbRAM.exe", "N/A"},
        {0, "N/A", "N/A", 5684, "C+G", "F:\\Documents\\NonDisclosureAgreement.exe", "N/A"},
        {0, "N/A", "N/A", 6700, "C+G", "G:\\RiotGames\\Valorant.exe", "N/A"}
    };

    system("cls");

    // Call printers
    printCurrDateTime();
    setCursorPosition(0, 2);
    printDisplay();

    setCursorPosition(0, 14);
    displayProcesses(processes);

    // Enter to exit
    consoleTextColor(7);
    cout << "\nPress Enter to exit...";
    cin.get();

    return 0;
}
