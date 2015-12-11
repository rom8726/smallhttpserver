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
            // TODO: refactor, make addService method

            ServicePtr iConfig(new AppConfig);
            ServicePtr iCache(new CacheService);

            AppConfig* config = static_cast<AppConfig*>(iConfig.get());
            CacheService* cache = static_cast<CacheService*>(iCache.get());

            if (config->init() == false) {
                std::cerr << "init app config failed!" << std::endl;
                return false;
            }

            if (config->isCachingEnabled()) {
                if (cache->init(config->getMemcachedServerPort()) == false) {
                    std::cerr << "init cache service failed!" << std::endl;
                    return false;
                }
            }

            m_services[AppConfig::getName()] = ServicePtr(std::move(iConfig));
            m_services[CacheService::getName()] = ServicePtr(std::move(iCache));

            return true;
        }


        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        SingletonDestroyer::~SingletonDestroyer() {
            if (m_pInstance)
                delete m_pInstance;
        }

        //----------------------------------------------------------------------
        void SingletonDestroyer::init(AppServicesFactory *p) {
            m_pInstance = p;
        }
    }
}