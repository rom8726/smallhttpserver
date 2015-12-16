#ifndef __COMMON__APP_CONFIG_H__
#define __COMMON__APP_CONFIG_H__

#include "tools.h"
#include "service_interface.h"

#include <string>
#include <memory>

namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        class AppConfig : public IService
        {
        public:
            AppConfig();
            AppConfig(const std::string& pathToConfig);
            virtual ~AppConfig() { }

            bool init();
            static const std::string getName() { return std::string("config"); }

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

            const std::string& getPathToConfig() const {
                return m_pathToConfig;
            }

            bool isLogging() const {
                return m_isLogging;
            }

            bool isCachingEnabled() const {
                return m_isCachingEnabled;
            }

            bool isDaemon() const {
                return m_isDaemon;
            }

            uint16_t getMemcachedServerPort() const {
                return m_memcachedSrvPort;
            }

            void setDaemon(bool isDaemon) {
                m_isDaemon = isDaemon;
            }

            void setPathToConfig(const std::string& path) {
                m_pathToConfig = path;
            }

        private:
            const uint16_t THREADS_MAX = 64;

            std::string m_srvIp;
            std::string m_rootDir;
            std::string m_defaultPage;
            std::string m_pathToConfig;

            uint16_t m_srvPort;
            uint16_t m_memcachedSrvPort;
            uint16_t m_threadsCount;

            bool m_isLogging;
            bool m_isCachingEnabled;
            bool m_isDaemon;
        };
    }
}

#endif //__COMMON__APP_CONFIG_H__
