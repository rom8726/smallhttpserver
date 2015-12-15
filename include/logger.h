#ifndef __COMMON__LOGGER_H__
#define __COMMON__LOGGER_H__

#include <string>
#include <stdio.h>

namespace Common {

    //----------------------------------------------------------------------
    class Logger {
    public:
        Logger();
        virtual ~Logger();

        virtual void log(const std::string& logMessage) const = 0;

        void setName(const std::string& name) { this->name = name; }

        const std::string& getName() const { return name; }

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
