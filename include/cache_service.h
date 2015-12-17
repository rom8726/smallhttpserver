#ifndef __COMMON__CACHE_SERVICE_H__
#define __COMMON__CACHE_SERVICE_H__

#include "service_interface.h"

#include <memory>
#include <libmemcached/memcached.h>

namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        class CacheService : public IService {
        public:
            CacheService();
            CacheService(uint16_t memcachedSrvPort);
            ~CacheService();

            bool init();
            bool init(uint16_t memcachedSrvPort);
            static const char* s_getName() { return "cache"; }
            virtual const char* getName() { return s_getName(); }

            bool store(const char* key, size_t key_len, const char* value, size_t val_len);
            char* load(const char* key, size_t key_len, size_t* return_value_length);

        private:
            memcached_server_st* m_memcSrvs;
            memcached_st* m_memcInst;
            uint16_t m_memcachedSrvPort;
        };
    }
}
#endif //__COMMON__CACHE_SERVICE_H__
