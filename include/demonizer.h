#ifndef __COMMON__DEMONIZER_H__
#define __COMMON__DEMONIZER_H__

#include "exceptions.h"

#include <signal.h>

namespace System {

    //----------------------------------------------------------------------
    class Demonizer {
    public:
        Demonizer();

        virtual ~Demonizer();

        void setName(const std::string &name) { this->name = name; }

        const std::string& getName() const { return name; }

        void setup() throw(DemonizerException);

        void stop() throw(DemonizerException);

        void stopWorker() throw(DemonizerException);

        void sendUserSignalToWorker() throw(DemonizerException);

        void startWithMonitoring(int(*startFunc)(void), int(*stopFunc)(void),
                                 int(*rereadCfgFun)(void));

        const static int CHILD_NEED_RESTART = 1;
        const static int CHILD_NEED_TERMINATE = 2;

        //private:
        static void signal_handler(int sig, siginfo_t *si, void *ptr);

        int workProc();

        static int (*m_sStartFunc)(void);

        static int (*m_sStopFunc)(void);

        static int (*m_sRereadCfgFun)(void);
    protected:
        std::string name;
    };

}
#endif /* __COMMON__DEMONIZER_H__ */
