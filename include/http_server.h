#ifndef __NETWORK_HTTP_SERVER_H__
#define __NETWORK_HTTP_SERVER_H__

#include "non_copyable.h"
#include "tools.h"
#include "http_request_interface.h"

#include <memory>
#include <vector>
#include <thread>
#include <string>

namespace Network {

    //----------------------------------------------------------------------
    class HttpServer final
            : private Common::NonCopyable
    {
    public:
        typedef std::vector<HttpRequestType> MethodPool;
        typedef std::function<void(IHttpRequestPtr)> OnRequestFunc;
        enum {
            MaxHeaderSize = static_cast<std::size_t>(-1), MaxBodySize = MaxHeaderSize
        };

        HttpServer(const std::string &address, uint16_t port,
                   uint16_t threadCount, const OnRequestFunc &onRequest,
                   const MethodPool &allowedMethodsArg = {HttpRequestType::GET},
                   std::size_t maxHeadersSize = MaxHeaderSize,
                   std::size_t maxBodySize = MaxBodySize);

    private:

        void (*threadDeleterFunct)(std::thread *t) = [](std::thread *t) {
            t->join();
            delete t;
        };
        typedef std::unique_ptr<std::thread, decltype(threadDeleterFunct)> ThreadPtr;
        typedef std::vector<ThreadPtr> ThreadsPool;
        ThreadsPool m_threadsPool;

        volatile bool m_isRun = true;
        Common::BoolFlagInvertor m_isRunInvertor;
    };

}


#endif  // !__NETWORK_HTTP_SERVER_H__
