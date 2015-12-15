#include <syslog.h>

#include "syslog_logger.h"

SyslogLogger::SyslogLogger() {
    // openlog("logger", LOG_CONS, LOG_DAEMON);
}

SyslogLogger::~SyslogLogger() {
}

/* set log name and open it */
void SyslogLogger::setName(std::string name) {
    Logger::setName(name);
    openlog(this->name.c_str(), LOG_CONS, LOG_DAEMON);
}

/* relization of log string to syslog */
void SyslogLogger::log(std::string logMessage) {
    syslog(LOG_INFO, logMessage.c_str());

}
