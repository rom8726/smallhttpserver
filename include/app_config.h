#ifndef __COMMON__APP_CONFIG_H__
#define __COMMON__APP_CONFIG_H__

#include "tools.h"
#include "service_interface.h"

#include <string>
#include <mutex>

namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        class AppConfig : public IService
        {
        public:
            virtual ~AppConfig() { }

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

            bool isDebug() const {
                return m_isDebug;
            }

            bool isCachingEnabled() const {
                return m_isCachingEnabled;
            }

            uint16_t getMemcachedServerPort() const {
                return m_memcachedSrvPort;
            }

            std::mutex& getStdOutMutex() {
                return m_stdOutMutex;
            }

        private:
            const uint16_t THREADS_MAX = 64;

            std::string m_srvIp;
            std::string m_rootDir;
            std::string m_defaultPage;

            uint16_t m_srvPort;
            uint16_t m_memcachedSrvPort;
            uint16_t m_threadsCount;

            bool m_isDebug;
            bool m_isCachingEnabled;

            std::mutex m_stdOutMutex;
        };
    }
}

#endif //__COMMON__APP_CONFIG_H__
