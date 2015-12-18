#ifndef __COMMON__LOGGER_H__
#define __COMMON__LOGGER_H__

#include <memory>
#include <string>
#include <stdio.h>
#include <string.h>

namespace Common {

    class Logger;
    typedef std::shared_ptr<Logger> LoggerPtr;

    //----------------------------------------------------------------------
    class Logger {
    public:
        Logger();
        virtual ~Logger();

        virtual void log(const std::string& logMessage) const = 0;
        virtual void err(const std::string& logMessage) const = 0;

        void setName(const std::string& name) { this->name = name; }

        const std::string& getName() const { return name; }

        static std::string itos(int data) {
            char buf[10];
            memset(buf, 0, 10);
            sprintf(buf, "%d", data);
            return buf;
        }

    protected:
        std::string name;
    };

}
#endif /* __COMMON__LOGGER_H__ */
