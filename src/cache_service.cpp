#include "cache_service.h"

#include <iostream>
#include <sstream>
#include <app_services.h>

namespace Common {

    namespace Services {

        //----------------------------------------------------------------------
        CacheService::CacheService()
                : m_memcInst(NULL), m_memcSrvs(NULL), m_memcachedSrvPort(0)
        {
        }

        //----------------------------------------------------------------------
        CacheService::CacheService(uint16_t memcachedSrvPort)
            : m_memcInst(NULL), m_memcSrvs(NULL), m_memcachedSrvPort(memcachedSrvPort)
        {
        }

        //----------------------------------------------------------------------
        CacheService::~CacheService() {
            if(m_memcInst)
                memcached_free(m_memcInst);
            if(m_memcSrvs)
                memcached_server_list_free(m_memcSrvs);
        }

        //----------------------------------------------------------------------
        bool CacheService::init() {

            if (this->isInitialized()) return true;

            try {

                if (m_memcachedSrvPort <= 1024) {
                    throw std::runtime_error("memcached server port must be > 1024!");
                }

                memcached_return rc;

                m_memcInst = memcached_create(NULL);
                m_memcSrvs = memcached_server_list_append(m_memcSrvs, "localhost", m_memcachedSrvPort, &rc);
                rc = memcached_server_push(m_memcInst, m_memcSrvs);

                if (rc != MEMCACHED_SUCCESS) {
                    std::stringstream ss("");
                    ss << "Couldn't add server: " << memcached_strerror(m_memcInst, rc);
                    throw std::runtime_error(ss.str());
                }

                this->setInitialized(true);

            } catch (const std::exception& ex) {
                AppServices::getLogger()->err(std::string(ex.what()));
            } catch (...) {}

            return isInitialized();
        }

        //----------------------------------------------------------------------
        bool CacheService::init(uint16_t memcachedSrvPort) {
            m_memcachedSrvPort = memcachedSrvPort;
            return init();
        }

        //----------------------------------------------------------------------
        bool CacheService::store(const char* key, size_t key_len, const char* value, size_t val_len) {
            memcached_return rc;
            rc = memcached_set(m_memcInst, key, key_len, value, val_len, (time_t) 0, (uint32_t) 0);

            if (rc != MEMCACHED_SUCCESS) {
//                std::stringstream ss("");
//                ss << "Couldn't store key: " << memcached_strerror(m_memcInst, rc);
//                throw std::runtime_error(ss.str());
                return false;
            }

            return true;
        }

        //----------------------------------------------------------------------
        char* CacheService::load(const char* key, size_t key_len, size_t* return_value_length) {
            uint32_t flags;
            memcached_return rc;

            char* response = memcached_get(m_memcInst, key, key_len, return_value_length, &flags, &rc);

            if (rc != MEMCACHED_SUCCESS) {
                *return_value_length = 0;
                return NULL;
            }

            return response;
        }
    }
}