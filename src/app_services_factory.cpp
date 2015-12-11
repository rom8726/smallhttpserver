#include <iostream>
#include "app_services_factory.h"

namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        AppServicesFactory *AppServicesFactory::m_pInstance = nullptr;
        SingletonDestroyer AppServicesFactory::destroyer;

        //----------------------------------------------------------------------
        AppServicesFactory &AppServicesFactory::getInstance() {
            if (!m_pInstance) {
                m_pInstance = new AppServicesFactory();
                destroyer.init(m_pInstance);
            }
            return *m_pInstance;
        }

        //----------------------------------------------------------------------
        bool AppServicesFactory::initAll() {
            if (m_appConfig.init() == false) {
                std::cerr << "init app config failed!" << std::endl;
                return false;
            }

            if (m_appConfig.isCachingEnabled()) {
                if (m_cacheService.init(m_appConfig.getMemcachedServerPort()) == false) {
                    std::cerr << "init cache service failed!" << std::endl;
                    return false;
                }
            }

            return true;
        }

        CacheService& AppServicesFactory::getCacheService() {
            if (m_appConfig.isCachingEnabled()) {
                return m_cacheService;
            }

            throw std::runtime_error("Caching is not enabled in config!");
        }

        //----------------------------------------------------------------------
        SingletonDestroyer::~SingletonDestroyer() {
            if (m_pInstance)
                delete m_pInstance;
        }

        void SingletonDestroyer::init(AppServicesFactory *p) {
            m_pInstance = p;
        }
    }
}