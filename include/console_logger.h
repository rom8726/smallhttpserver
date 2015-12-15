#ifndef __COMMON__LOGGER_H__
#define __COMMON__LOGGER_H__

#include "logger.h"

namespace Common {

    class ConsoleLogger : public Logger {
    public:
        ConsoleLogger();
        virtual ~ConsoleLogger();

        void log(std::string logMessage);

        void log_n(char c);
    };

}
#endif /* __COMMON__LOGGER_H__ */
