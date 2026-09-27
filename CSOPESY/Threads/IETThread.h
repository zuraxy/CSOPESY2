#pragma once
#include <iostream>
#include <thread>

class IETThread {
    public:
        IETThread() = default;
        ~IETThread() = default;

        void start();
        static void sleep(int ms);

    protected:
        virtual void run() = 0;
};