#ifndef __COMMON__SYSLOGLOGGER_H__
#define __COMMON__SYSLOGLOGGER_H__

#include "logger.h"

namespace Common {

    class SyslogLogger : public Logger {
    public:
        SyslogLogger();

        void log(std::string logMessage);

        void setName(std::string name);

        virtual ~SyslogLogger();
    };

}
#endif /* __COMMON__SYSLOGLOGGER_H__ */
