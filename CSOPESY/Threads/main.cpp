//oneto one implementation as discussed (added infinite loop to keep main thread alive)
#include <iostream>
#include "HelloWorldThread.h"
#include "IETThread.h"
#include <thread>

void createHWThreads() {
    for (int i = 0; i < 20; i++) {
        HelloWorldThread* thread = new HelloWorldThread(i);
        thread->start();
    }
}

int main() {
    createHWThreads();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}