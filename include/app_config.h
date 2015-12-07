#ifndef __COMMON__APP_CONFIG_H__
#define __COMMON__APP_CONFIG_H__

#include "tools.h"

#include <string>
#include <mutex>

namespace Common {

    namespace Config {

        //----------------------------------------------------------------------
        class SingletonDestroyer;

        class AppConfig final : public NonCopyable
        {
        public:
            static AppConfig& getInstance();
            bool init();

            const std::string& getServerIp() const {
                return m_srvIp;
            }

            uint16_t getServerPort() const {
                return m_srvPort;
            }

            uint16_t getThreadsCnt() const {
                return m_threadsCount;
            }

            const std::string& getRootDir() const {
                return m_rootDir;
            }

            const std::string& getDefaultPage() const {
                return m_defaultPage;
            }

            bool getIsDebug() const {
                return m_isDebug;
            }

            std::mutex& getStdOutMutex() {
                return m_stdOutMutex;
            }

        protected:
            ~AppConfig() { }
            friend class SingletonDestroyer;

        private:
            static AppConfig *m_pInstance;
            static SingletonDestroyer destroyer;

            const uint16_t THREADS_MAX = 64;

            std::string m_srvIp;
            uint16_t m_srvPort;
            uint16_t m_threadsCount;
            std::string m_rootDir;
            std::string m_defaultPage;
            bool m_isDebug;
            std::mutex m_stdOutMutex;
        };

        //----------------------------------------------------------------------
        class SingletonDestroyer {
        private:
            AppConfig *m_pInstance = nullptr;
        public:
            virtual ~SingletonDestroyer();
            void init(AppConfig *p);
        };
    }
}

#endif //__COMMON__APP_CONFIG_H__
