#include "console_logger.h"
#include <iostream>

namespace Common {

    ConsoleLogger::ConsoleLogger() {
    }

    ConsoleLogger::~ConsoleLogger() {
    }

    /*
    * log() realization with cout
    */
    void ConsoleLogger::log(std::string logMessage) {

        std::cout << logMessage << std::endl;

    }

    void ConsoleLogger::log_n(char c) {
        std::cout << c;
    }
}