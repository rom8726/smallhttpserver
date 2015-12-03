#include "http_request.h"

#include <sstream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>


namespace Network {

    void HttpRequest::initUri() const {
        if (m_uri)
            return;
        auto *This = const_cast<HttpRequest *>(this);
        if (!(This->m_uri = evhttp_request_get_evhttp_uri(m_request)))
            throw HttpRequestException("Failed to get uri.");
    }

    //----------------------------------------------------------------------
    void HttpRequest::initOutputBuf() const {
        if (m_outputBuf)
            return;
        auto *This = const_cast<HttpRequest *>(this);
        if (!(This->m_outputBuf = evhttp_request_get_output_buffer(m_request)))
            throw HttpRequestException("Failed to get output buffer.");
    }

    //----------------------------------------------------------------------
    void HttpRequest::initInputHeaders() const {
        if (m_inputHeaders != nullptr)
            return;
        auto *This = const_cast<HttpRequest *>(this);
        if (!(This->m_inputHeaders = evhttp_request_get_input_headers(m_request)))
            throw HttpRequestException("Failed to get http input headers.");
    }

    //----------------------------------------------------------------------
    void HttpRequest::initOutputHeaders() const {
        if (m_outputHeaders != nullptr)
            return;
        auto *This = const_cast<HttpRequest *>(this);
        if (!(This->m_outputHeaders = evhttp_request_get_output_headers(m_request)))
            throw HttpRequestException("Failed to get http output headers.");
    }

    //----------------------------------------------------------------------
    RequestType HttpRequest::getRequestType() const {
        switch (evhttp_request_get_command(m_request)) {
            case EVHTTP_REQ_GET :
                return RequestType::GET;
            case EVHTTP_REQ_POST :
                return RequestType::POST;
            case EVHTTP_REQ_HEAD :
                return RequestType::HEAD;
            case EVHTTP_REQ_PUT :
                return RequestType::PUT;
            default :
                break;
        }
        throw HttpRequestException("Unknown request type.");
    }

    //----------------------------------------------------------------------
    const std::string HttpRequest::getHeaderAttr(const char *attrName) const {
        this->initInputHeaders();
        const char *ret = evhttp_find_header(m_inputHeaders, attrName);
        return ret ? ret : "";
    }

    //----------------------------------------------------------------------
    std::size_t HttpRequest::getContentSize() const {
        if (m_inputBuf == nullptr) {
            auto *This = const_cast<HttpRequest *>(this);
            if (!(This->m_inputBuf = evhttp_request_get_input_buffer(m_request)))
                throw HttpRequestException("Failed to get input buffer.");
        }
        return evbuffer_get_length(m_inputBuf);
    }

    //----------------------------------------------------------------------
    void HttpRequest::getContent(void *buf, std::size_t len, bool remove) const {
        if (len > this->getContentSize())
            throw HttpRequestException("Required length of data buffer more than exists.");

        if (remove) {
            if (evbuffer_remove(m_inputBuf, buf, len) == -1)
                throw HttpRequestException("Failed to get input data.");
            return;
        }

        if (evbuffer_copyout(m_inputBuf, buf, len) == -1)
            throw HttpRequestException("Failed to get input data.");
    }

    //----------------------------------------------------------------------
    const std::string HttpRequest::getPath() const {
        this->initUri();
        const char *ret = evhttp_uri_get_path(m_uri);
        return ret ? ret : "";
    }

    //----------------------------------------------------------------------
    const HttpRequestParams HttpRequest::getParams() const {
        this->initUri();
        HttpRequestParams params;
        const char *query = evhttp_uri_get_query(m_uri);
        if (!query)
            return std::move(params);

        std::stringstream ioStream;
        ioStream << query;
        for (std::string s; ioStream;) {
            std::getline(ioStream, s, '&');
            auto pos = s.find('=');
            if (pos != std::string::npos)
                params[s.substr(0, pos)] = s.substr(pos + 1);
            else
                params[s] = "";
        }
        return std::move(params);
    }

    //----------------------------------------------------------------------
    void HttpRequest::setResponseAttr(const std::string &name, const std::string &val) {
        this->initOutputHeaders();
        if (evhttp_add_header(m_outputHeaders, name.c_str(), val.c_str()) == -1)
            throw HttpRequestException("Failed to set response header attribute.");
    }

    //----------------------------------------------------------------------
    void HttpRequest::setResponseCode(int code) {
        m_responseCode = code;
    }

    //----------------------------------------------------------------------
    void HttpRequest::setResponseString(const std::string &str) {
        this->initOutputBuf();
        if (evbuffer_add_printf(m_outputBuf, str.c_str()) == -1)
            throw HttpRequestException("Failed to make response.");
    }

    //----------------------------------------------------------------------
    void HttpRequest::setResponseBuf(const void *data, std::size_t bytes) {
        this->initOutputBuf();
        void *newData = ::operator new(bytes);
        std::memcpy(newData, data, bytes);
        if (evbuffer_add_reference(m_outputBuf, newData, bytes, &HttpRequest::freeBuf, nullptr) == -1) {
            ::operator delete(newData);
            throw HttpRequestException("Failed to make response.");
        }
    }

    //----------------------------------------------------------------------
    void HttpRequest::setResponseFile(const std::string &fileName) {
        this->initOutputBuf();

        auto fileDeleterFunct = [](int *f) {
            if (*f != -1) close(*f);
            delete f;
        };

        std::unique_ptr<int, decltype(fileDeleterFunct)> file(new int(open(fileName.c_str(), 0)),
                                                              fileDeleterFunct);
        if (*file == -1)
            throw HttpRequestException(HTTP_NOTFOUND, "Could not find content for uri.");

        ev_off_t Length = lseek(*file, 0, SEEK_END);
        if (Length == -1 || lseek(*file, 0, SEEK_SET) == -1)
            throw HttpRequestException("Failed to calc file size.");
        if (evbuffer_add_file(m_outputBuf, *file, 0, Length) == -1)
            throw HttpRequestException("Failed to make response.");

        *file.get() = -1;
    }
}
