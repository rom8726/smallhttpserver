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
            virtual bool isInitialized() { return m_isInitialized; }

            static const char* s_getName() { return ""; }
            virtual const char* getName() = 0;/*{ return s_getName(); }*/

        protected:
            virtual bool setInitialized(bool initFlag) { m_isInitialized = initFlag; }

        private:
            bool m_isInitialized;
        };
    }
}

#endif //__COMMON__SERVICE_INTERFACE_H__
