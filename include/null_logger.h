#ifndef __COMMON__NULLLOGGER_H__
#define __COMMON__NULLLOGGER_H__

#include "logger.h"

namespace Common {

    //----------------------------------------------------------------------
    class NullLogger : public Logger {
    public:
        NullLogger();
        virtual ~NullLogger();

        void log(const std::string& logMessage) const { (void) logMessage; };
    };

}
#endif /* __COMMON__NULLLOGGER_H__ */
