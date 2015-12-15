#include <syslog.h>

#include "syslog_logger.h"

namespace Common {

    //----------------------------------------------------------------------
    SyslogLogger::SyslogLogger() {
        // openlog("logger", LOG_CONS, LOG_DAEMON);
    }

    //----------------------------------------------------------------------
    SyslogLogger::~SyslogLogger() {
    }

    //----------------------------------------------------------------------
    void SyslogLogger::setName(const std::string& name) {
        Logger::setName(name);
        openlog(this->name.c_str(), LOG_CONS, LOG_DAEMON);
    }

    //----------------------------------------------------------------------
    void SyslogLogger::log(const std::string& logMessage) const {
        syslog(LOG_INFO, logMessage.c_str(), "");
    }
}