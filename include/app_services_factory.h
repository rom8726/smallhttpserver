#ifndef __COMMON__APP_SERVICES_FACTORY_H__
#define __COMMON__APP_SERVICES_FACTORY_H__

#include "non_copyable.h"
#include "app_config.h"
#include "cache_service.h"

namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        class SingletonDestroyer;

        class AppServicesFactory final : public NonCopyable {
        public:
            static AppServicesFactory& getInstance();
            bool initAll();

            AppConfig& getConfig() { return m_appConfig; }
            CacheService& getCacheService();

        protected:
            ~AppServicesFactory() { }
            friend class SingletonDestroyer;
        private:
            static AppServicesFactory *m_pInstance;
            static SingletonDestroyer destroyer;

            AppConfig m_appConfig;
            CacheService m_cacheService;
        };

        //----------------------------------------------------------------------
        class SingletonDestroyer {
        private:
            AppServicesFactory *m_pInstance = nullptr;
        public:
            virtual ~SingletonDestroyer();
            void init(AppServicesFactory *p);
        };
    }
}

#endif //__COMMON__APP_SERVICES_FACTORY_H__
