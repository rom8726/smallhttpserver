#ifndef __COMMON__LOGGER_H__
#define __COMMON__LOGGER_H__

#include <string>
#include <stdio.h>

namespace Common {

    class Logger {
    public:
        Logger();
        virtual ~Logger();

        virtual void log(std::string logMessage) = 0;

        void setName(std::string name) { this->name = name; }

        std::string &getName() { return name; }

        static std::string itos(int data) {
            char buf[10];
            sprintf(buf, "%d", data);
            return buf;
        }

    protected:
        std::string name;
    };

}
#endif /* __COMMON__LOGGER_H__ */
