#include "console_logger.h"
#include <iostream>

namespace Common {

    //----------------------------------------------------------------------
    ConsoleLogger::ConsoleLogger() {
    }

    //----------------------------------------------------------------------
    ConsoleLogger::~ConsoleLogger() {
    }

    //----------------------------------------------------------------------
    void ConsoleLogger::log(const std::string& logMessage) const {
        std::cout << logMessage << std::endl;
    }

    //----------------------------------------------------------------------
    void ConsoleLogger::err(const std::string& logMessage) const {
        std::cerr << logMessage << std::endl;
    }

    //----------------------------------------------------------------------
    void ConsoleLogger::log_n(char c) const {
        std::cout << c;
    }
}