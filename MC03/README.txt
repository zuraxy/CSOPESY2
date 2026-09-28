CSOPESY MARQUEE CONSOLE
=======================

Group members
-------------
Dela Cruz, Karl Matthew
Aquino, Bon Windel
Espinosa, Jose Miguel
Pineda, Dencel Angelo

Entry source file
-----------------
main.cpp contains the int main() function. C++ does not require an entry class.

Run without the batch file
--------------------------
This is a Windows console program. Open Command Prompt or PowerShell in this
Exercise folder, then run the included executable:

  Command Prompt:  csopesy_marquee.exe
  PowerShell:      .\csopesy_marquee.exe

If the executable needs to be rebuilt, install MinGW-w64 and make sure g++ is
available in PATH. From this folder, compile it without run.bat using:

  g++ -std=c++17 -Wall -Wextra -pedantic main.cpp CommandInterpreter.cpp Marquee.cpp ConsoleUI.cpp -lwinmm -o csopesy_marquee.exe

Then run csopesy_marquee.exe using the command shown above.

Program commands
----------------
help                 Show the command list and descriptions.
start_marquee        Start the bouncing animation.
stop_marquee         Pause the animation.
set_text <text>      Replace the ASCII art with your text.
set_speed <ms>       Set the redraw interval in milliseconds (1-60000).
exit                 Close the program.
