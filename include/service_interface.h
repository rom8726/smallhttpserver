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

            //static const std::string getName() { return std::string(""); }

            static const char* s_getTypeName() { return typeid(IService).name(); }
            virtual const char* getTypeName() = 0;/*{ return s_getTypeName(); }*/

        protected:
            virtual bool setInitialized(bool initFlag) { m_isInitialized = initFlag; }

        private:
            bool m_isInitialized;
        };
    }
}

#endif //__COMMON__SERVICE_INTERFACE_H__
