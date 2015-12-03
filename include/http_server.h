#ifndef __NETWORK_HTTP_SERVER_H__
#define __NETWORK_HTTP_SERVER_H__

#include "non_copyable.h"
#include "exceptions.h"
#include "tools.h"
#include "http_request.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <thread>
#include <string>

namespace Network {

    class HttpServer final
            : private Common::NonCopyable {
    public:
        typedef std::vector<IHttpRequest::Type> MethodPool;
        typedef std::function<void(IHttpRequestPtr)> OnRequestFunc;
        enum {
            MaxHeaderSize = static_cast<std::size_t>(-1), MaxBodySize = MaxHeaderSize
        };

        HttpServer(const std::string& address, std::uint16_t port,
                   std::uint16_t threadCount, const OnRequestFunc& onRequest,
                   const MethodPool& allowedMethodsArg = {IHttpRequest::Type::GET},
                   std::size_t maxHeadersSize = MaxHeaderSize,
                   std::size_t maxBodySize = MaxBodySize);

    private:
        volatile bool m_isRun = true;

        void (*threadDeleterFunct)(std::thread* t) = [](std::thread* t) {
            t->join();
            delete t;
        };;
        typedef std::unique_ptr<std::thread, decltype(threadDeleterFunct)> ThreadPtr;
        typedef std::vector<ThreadPtr> ThreadsPool;
        ThreadsPool m_threadsPool;

        Common::BoolFlagInvertor m_isRunInvertor;
    };

}


#endif  // !__NETWORK_HTTP_SERVER_H__
