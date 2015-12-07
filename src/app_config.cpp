#include "app_config.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


namespace Common {

    namespace Config {

        //----------------------------------------------------------------------
        AppConfig *AppConfig::m_pInstance = nullptr;
        SingletonDestroyer AppConfig::destroyer;

        //----------------------------------------------------------------------
        AppConfig &AppConfig::getInstance() {
            if (!m_pInstance) {
                m_pInstance = new AppConfig();
                destroyer.init(m_pInstance);
            }
            return *m_pInstance;
        }

        bool AppConfig::init() {

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

            } catch (const std::exception &ex) {
                std::cerr << ex.what() << std::endl;
                return false;
            } catch (...) {
                return false;
            }

            return true;
        }

        //----------------------------------------------------------------------
        SingletonDestroyer::~SingletonDestroyer() {
            if (m_pInstance)
                delete m_pInstance;
        }

        void SingletonDestroyer::init(AppConfig *p) {
            m_pInstance = p;
        }
    }
}