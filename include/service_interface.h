#ifndef __COMMON__SERVICE_INTERFACE_H__
#define __COMMON__SERVICE_INTERFACE_H__

#include <string>


namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        class IService {
        public:
            IService()
                : m_isInitialized(false)
            {}

            virtual bool init () = 0;
            virtual bool isInitialized() { return m_isInitialized; }

            static const std::string getName() { return std::string(""); }

        protected:
            virtual bool setInitialized(bool initFlag) { m_isInitialized = initFlag; }

        private:
            bool m_isInitialized;
        };
    }
}

#endif //__COMMON__SERVICE_INTERFACE_H__
