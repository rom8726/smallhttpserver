#ifndef __COMMON__SYSLOGLOGGER_H__
#define __COMMON__SYSLOGLOGGER_H__

#include "logger.h"

namespace Common {

    //----------------------------------------------------------------------
    class SyslogLogger : public Logger {
    public:
        SyslogLogger();

        void log(const std::string& logMessage) const;

        void setName(const std::string& name);

        virtual ~SyslogLogger();
    };

}
#endif /* __COMMON__SYSLOGLOGGER_H__ */
