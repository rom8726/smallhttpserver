#include "app_services.h"

namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        AppServices *AppServices::m_pInstance = nullptr;
        LoggerPtr AppServices::m_logger = nullptr;
        SingletonDestroyer AppServices::destroyer;

        //----------------------------------------------------------------------
        AppServices &AppServices::getInstance() {
            if (!m_pInstance) {
                m_pInstance = new AppServices();
                destroyer.init(m_pInstance);
                if (!m_logger.get()) {
                    m_logger.reset(new ConsoleLogger);
                }
            }
            return *m_pInstance;
        }

        //----------------------------------------------------------------------
        SingletonDestroyer::~SingletonDestroyer() {
            if (m_pInstance)
                delete m_pInstance;
        }

        //----------------------------------------------------------------------
        void SingletonDestroyer::init(AppServices *p) {
            m_pInstance = p;
        }
    }
}