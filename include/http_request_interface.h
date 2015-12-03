#ifndef __I_NETWORK_HTTP_REQUEST_H__
#define __I_NETWORK_HTTP_REQUEST_H__

#include "exceptions.h"

#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace Network {

    //----------------------------------------------------------------------
    class IHttpRequest;

    typedef std::shared_ptr<IHttpRequest> IHttpRequestPtr;
    typedef std::unordered_map<std::string, std::string> HttpRequestParams;
    enum class HttpRequestType {
        HEAD, GET, PUT, POST
    };

    //----------------------------------------------------------------------
    class IHttpRequest {
    public:

        virtual ~IHttpRequest() { }

        virtual HttpRequestType getRequestType() const = 0;

        virtual const std::string getHeaderAttr(const char *attrName) const = 0;

        virtual std::size_t getContentSize() const = 0;

        virtual void getContent(void *buf, std::size_t len, bool remove) const = 0;

        virtual const std::string getPath() const = 0;

        virtual const HttpRequestParams getParams() const = 0;

        virtual void setResponseAttr(const std::string &name, const std::string &val) = 0;

        virtual void setResponseCode(int code) = 0;

        virtual void setResponseString(const std::string &str) = 0;

        virtual void setResponseBuf(const void *data, std::size_t bytes) = 0;

        virtual void setResponseFile(const std::string &fileName) = 0;
    };

}

#endif  // !__I_NETWORK_HTTP_REQUEST_H__
