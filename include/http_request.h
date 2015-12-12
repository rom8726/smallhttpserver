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
        void initUri() const;
        void initInputBuf() const;
        void initOutputBuf() const;
        void initInputHeaders() const;
        void initOutputHeaders() const;

        static void freeBuf(const void *data, std::size_t datalen, void *extra) {
            ::operator delete(const_cast<void *>(data));
        }

        virtual const std::string getPath() const;
        virtual const HttpRequestParams getParams() const;
        virtual HttpRequestType getRequestType() const;
        virtual const std::string getHeaderAttr(const char *attrName) const;

        virtual std::size_t getInputContentSize() const;
        virtual std::size_t getOutputContentSize() const;
        virtual bool getInputContent(void *buf, std::size_t len, bool remove) const;
        virtual bool getOutputContent(void *buf, std::size_t len, bool remove) const;

        virtual void setResponseAttr(const std::string &name, const std::string &val);
        virtual void setResponseCode(int code);
        virtual void setResponseString(const std::string &str);
        virtual void setResponseBuf(const void *data, std::size_t bytes);
        virtual void setResponseFile(const std::string &fileName);

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
