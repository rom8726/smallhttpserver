#ifndef __COMMON__APP_SERVICES_FACTORY_H__
#define __COMMON__APP_SERVICES_FACTORY_H__

#include "non_copyable.h"
#include "app_config.h"
#include "cache_service.h"

#include <sstream>
#include <memory>
#include <map>


namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        class SingletonDestroyer;

        class AppServicesFactory final : public NonCopyable {
        public:
            static AppServicesFactory& getInstance();
            bool initAll();

            template <class T>
            T* getService() {
                static_assert(std::is_base_of<IService, T>::value, "AppServicesFactory::getService: class T not derived from IService");

                std::string name = T::getName();
                if (m_services.find(name) != m_services.end()) {
                    IService* srv = m_services[name].get();
                    return static_cast<T*>(srv);
                }

                std::stringstream ss("");
                ss << "Service " << name << " not found!";
                throw std::runtime_error(ss.str());
            }

        protected:
            ~AppServicesFactory() { }
            friend class SingletonDestroyer;
        private:
            static AppServicesFactory *m_pInstance;
            static SingletonDestroyer destroyer;

            typedef std::unique_ptr<IService> ServicePtr;
            typedef std::map<std::string, ServicePtr> ServicesMap;
            ServicesMap m_services;
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
