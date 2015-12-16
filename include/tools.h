#ifndef __COMMON_TOOLS_H__
#define __COMMON_TOOLS_H__

#include "non_copyable.h"
#include "exceptions.h"
#include "app_services.h"
#include "app_config.h"


namespace Common {

    //----------------------------------------------------------------------
    class BoolFlagInvertor final
            : private NonCopyable {
    public:
        BoolFlagInvertor(volatile bool *pFlag)
                : m_pFlag(pFlag) {
        }

        ~BoolFlagInvertor() {
            *m_pFlag = !*m_pFlag;
        }

    private:
        volatile bool *m_pFlag;
    };

}


//----------------------------------------------------------------------
namespace System {

    //----------------------------------------------------------------------
    class SystemTools final : private Common::NonCopyable {
    public:
        static std::string calculateFilenameToStorePID(const std::string& processName);

        //check if the process already running
        static bool checkRunningAndSavePID(const std::string& processName) throw(SystemException);

        static bool checkRunningAndSavePID(const std::string& processName, int pid) throw(SystemException);

        //find pid in pid file by given process
        static int findPID(const std::string& processName) throw(SystemException);

        //kills process by pid
        static void kill(int pid);

        static void killWithSignal(int pid, int sig);
    };
}

#endif  // !__COMMON_TOOLS_H__
