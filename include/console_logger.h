#ifndef __COMMON__CONSOLE_LOGGER_H__
#define __COMMON__CONSOLE_LOGGER_H__

#include "logger.h"

#include <string>

namespace Common {

    class ConsoleLogger : public Logger {
    public:
        ConsoleLogger();
        virtual ~ConsoleLogger();

        void log(const std::string& logMessage) const;

        void log_n(char c) const;
    };

}
#endif /* __COMMON__CONSOLE_LOGGER_H__ */
