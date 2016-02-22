#ifndef __NETWORK_HTTP_REQUEST_H__
#define __NETWORK_HTTP_REQUEST_H__

#include "non_copyable.h"
#include "http_request_interface.h"
#include "tools.h"

#include <string>
#include <evhttp.h>


namespace Network {

    //----------------------------------------------------------------------
    class HttpRequest final
            : private Common::NonCopyable, public IHttpRequest {
    public:
        HttpRequest(evhttp_request *request)
                : m_request(request) { }

        int getResponseCode() const {
            return m_responseCode;
        }

    private:
        void initUri() const throw(HttpRequestException);
        void initInputBuf() const throw(HttpRequestException);
        void initOutputBuf() const throw(HttpRequestException);
        void initInputHeaders() const throw(HttpRequestException);
        void initOutputHeaders() const throw(HttpRequestException);

        static void freeBuf(const void *data, std::size_t datalen, void *extra) {
            ::operator delete(const_cast<void *>(data));
        }

        virtual const std::string getPath() const;
        virtual const HttpRequestParams getParams() const;
        virtual HttpRequestType getRequestType() const throw(HttpRequestException);
        virtual const std::string getHeaderAttr(const char *attrName) const;

        virtual std::size_t getInputContentSize() const;
        virtual std::size_t getOutputContentSize() const;
        virtual bool getInputContent(void *buf, std::size_t len, bool remove) const throw(HttpRequestException);
        virtual bool getOutputContent(void *buf, std::size_t len, bool remove) const throw(HttpRequestException);

        virtual void setResponseCode(int code);
        virtual void setResponseAttr(const std::string &name, const std::string &val) throw(HttpRequestException);
        virtual void setResponseString(const std::string &str) throw(HttpRequestException);
        virtual void setResponseBuf(const void *data, std::size_t bytes) throw(HttpRequestException);
        virtual void setResponseFile(const std::string &fileName) throw(HttpRequestException);

        evhttp_request *m_request;
        evkeyvalq *m_inputHeaders = nullptr;
        evkeyvalq *m_outputHeaders = nullptr;
        evbuffer *m_inputBuf = nullptr;
        evbuffer *m_outputBuf = nullptr;
        const evhttp_uri *m_uri = nullptr;
        int m_responseCode = HTTP_OK;
    };
}

#endif //__NETWORK_HTTP_REQUEST_H__
