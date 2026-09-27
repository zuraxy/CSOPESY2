#include "HelloWorldThread.h"
#include <iostream>

HelloWorldThread::HelloWorldThread(int id) {
    this->id = id;
}

HelloWorldThread::~HelloWorldThread() {}

void HelloWorldThread::run() {
    while (true) {
        std::cout << "This is a hello world in a thread # " << this->id << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}