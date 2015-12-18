#ifndef __COMMON__SERVICE_INTERFACE_H__
#define __COMMON__SERVICE_INTERFACE_H__

#include <string>
#include <memory>

namespace Common {

    namespace Services {

        class IService;
        typedef std::shared_ptr<IService> ServicePtr;

        //----------------------------------------------------------------------
        class IService {
        public:
            IService()
                : m_isInitialized(false)
            {}

            virtual bool init () = 0;
            virtual inline bool isInitialized() { return m_isInitialized; }

            virtual /*inline*/ const char* getTypeName() = 0; /*{ return typeid(IService).name(); }*/

        protected:
            virtual void setInitialized(bool initFlag) { m_isInitialized = initFlag; }

        private:
            bool m_isInitialized;
        };
    }
}

#endif //__COMMON__SERVICE_INTERFACE_H__
