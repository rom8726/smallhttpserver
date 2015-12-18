#include "app_config.h"

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        AppConfig::AppConfig()
            : m_pathToConfig("./config.json"), m_isLogging(false),
              m_isCachingEnabled(false), m_isDaemon(false)
        {
        }

        //----------------------------------------------------------------------
        AppConfig::AppConfig(const std::string& pathToConfig)
            : m_pathToConfig(pathToConfig), m_isLogging(false),
              m_isCachingEnabled(false), m_isDaemon(false)
        {
        }

        //----------------------------------------------------------------------
        bool AppConfig::init() {

            if (this->isInitialized()) return true;

            try {

                if (access(m_pathToConfig.c_str(), F_OK) != 0) {
                    throw std::runtime_error("config.json is not found!");
                }

                std::ifstream file(m_pathToConfig);
                if (file.is_open()) {

                    boost::property_tree::ptree pt;
                    boost::property_tree::read_json(file, pt);

                    m_srvIp = pt.get<std::string>("server-ip");
                    // TODO: validate ip

                    m_srvPort = pt.get<uint16_t>("server-port");

                    m_threadsCount = pt.get<uint16_t>("threads-count");
                    if (m_threadsCount > THREADS_MAX) {
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

                    m_isLogging = pt.get<bool>("logging-enable");

                    m_isCachingEnabled = pt.get<bool>("cache-enable");
                    if (m_isCachingEnabled) {
                        m_memcachedSrvPort = pt.get<uint16_t>("memcached-server-port");
                        if (m_memcachedSrvPort <= 1024) {
                            throw std::runtime_error("memcached server port must be > 1024!");
                        }
                    } else {
                        m_memcachedSrvPort = 0;
                    }


                    BOOST_FOREACH(const boost::property_tree::ptree::value_type& child, pt.get_child("extension-contenttype")) {
                        auto ext = child.second.get<std::string>("ext");
                        auto type = child.second.get<std::string>("type");
                        m_contentTypes.insert(std::make_pair(ext, type));
                    }


                    this->setInitialized(true);

                } else {
                    throw std::runtime_error("config.json open failed!");
                }

            } catch (const std::exception &ex) {
                AppServices::getLogger()->err(ex.what());
            } catch (...) {
            }

            return isInitialized();
        }

        //----------------------------------------------------------------------
        const std::string AppConfig::getContentTypeFromFileName(const std::string& fileName) const {
            auto pos = fileName.rfind('.');
            if (pos == std::string::npos)
                return "";

            return this->getContentTypeByExtension(fileName.substr(pos + 1));
        }

        //----------------------------------------------------------------------
        const std::string AppConfig::getContentTypeByExtension(const std::string& extension) const {
            if (m_contentTypes.find(extension) == m_contentTypes.end()) {
                return "";
            }

            return m_contentTypes.at(extension);
        }
    }
}