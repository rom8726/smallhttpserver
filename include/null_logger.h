#ifndef __COMMON__NULLLOGGER_H__
#define __COMMON__NULLLOGGER_H__

#include "logger.h"

namespace Common {

    class NullLogger : public Logger {
    public:
        NullLogger();
        virtual ~NullLogger();

        void log(std::string logMessage) { (void) logMessage; };
    };

}
#endif /* __COMMON__NULLLOGGER_H__ */
