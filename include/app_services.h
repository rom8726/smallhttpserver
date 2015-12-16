#ifndef __COMMON__APP_SERVICES_FACTORY_H__
#define __COMMON__APP_SERVICES_FACTORY_H__

#include "non_copyable.h"
#include "app_config.h"
#include "cache_service.h"
#include "console_logger.h"

#include <sstream>
#include <map>


namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        class SingletonDestroyer;

        class AppServices final : public NonCopyable {
        public:
            static AppServices& getInstance();
            void clear() { m_services.clear(); }

            template <class T>
            void addService(ServicePtr servicePtr) {
                static_assert(std::is_base_of<IService, T>::value, "AppServices::addService: class T not derived from IService");
                m_services[T::getName()] = servicePtr;
            }

            template <class T>
            T* getService() {
                static_assert(std::is_base_of<IService, T>::value, "AppServices::getService: class T not derived from IService");

                const std::string name = T::getName();
                if (m_services.find(name) != m_services.end()) {
                    IService* srv = m_services[name].get();
                    if (!srv->isInitialized()) {
                        throw std::runtime_error(std::string("Service ") + name + std::string(" is not initialized!"));
                    }
                    return static_cast<T*>(srv);
                }

                throw std::runtime_error(std::string("Service ") + name + std::string(" not found!"));
            }

            static LoggerPtr& getLogger() {
                if (!m_logger)
                    m_logger.reset(new ConsoleLogger);
                return m_logger;
            }

            static void setLogger(LoggerPtr logger) { m_logger = logger; }

        protected:
            ~AppServices() { }
            friend class SingletonDestroyer;
        private:
            static AppServices *m_pInstance;
            static SingletonDestroyer destroyer;

            typedef std::map<std::string, ServicePtr> ServicesMap;
            ServicesMap m_services;

            static LoggerPtr m_logger;
        };

        //----------------------------------------------------------------------
        class SingletonDestroyer {
        private:
            AppServices *m_pInstance = nullptr;
        public:
            virtual ~SingletonDestroyer();
            void init(AppServices *p);
        };
    }
}

#endif //__COMMON__APP_SERVICES_FACTORY_H__
