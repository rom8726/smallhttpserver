#include "app_config.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        bool AppConfig::init() {

            if (this->isInitialized()) return true;

            try {

                if (access("config.json", F_OK) != 0) {
                    throw std::runtime_error("config.json is not found!");
                }

                std::ifstream file("config.json");
                boost::property_tree::ptree pt;
                boost::property_tree::read_json(file, pt);

                m_srvIp = pt.get<std::string>("server-ip");
                // TODO: validate ip

                m_srvPort = pt.get<uint16_t>("server-port");

                m_threadsCount = pt.get<uint16_t>("threads-count");
                if (m_threadsCount > 64) {
                    std::stringstream str("");
                    str << "threads-count param is too big(" << THREADS_MAX << " is max)";
                    throw std::runtime_error(str.str());
                } else if (m_threadsCount == 0) {
                    throw std::runtime_error("threads-count param is 0!");
                }

                m_rootDir = pt.get<std::string>("root-dir");
                if (access(m_rootDir.c_str(), F_OK) != 0) {
                    throw std::runtime_error("root dir is not exist!");
                }
                if (m_rootDir.back() == '/') {
                    m_rootDir = m_rootDir.substr(0, m_rootDir.length() - 1);
                }

                m_defaultPage = pt.get<std::string>("default-page");
                if (access((m_rootDir + '/' + m_defaultPage).c_str(), F_OK) != 0) {
                    throw std::runtime_error("default page is not exist!");
                }

                m_isDebug = pt.get<bool>("is-debug");

                m_isCachingEnabled = pt.get<bool>("cache-enable");
                if (m_isCachingEnabled) {
                    m_memcachedSrvPort = pt.get<uint16_t>("memcached-server-port");
                    if (m_memcachedSrvPort <= 1024) {
                        throw std::runtime_error("memcached server port must be > 1024!");
                    }
                } else {
                    m_memcachedSrvPort = 0;
                }

                this->setInitialized(true);

            } catch (const std::exception &ex) {
                std::cerr << ex.what() << std::endl;
            } catch (...) {
            }

            return isInitialized();
        }
    }
}