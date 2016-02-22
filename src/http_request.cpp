#include "http_request.h"

#include <sstream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

using namespace Common::Services;


namespace Network {

    //----------------------------------------------------------------------
    void HttpRequest::initUri() const throw(HttpRequestException) {

        if (m_uri == nullptr) {
            auto *This = const_cast<HttpRequest *>(this);
            if (!(This->m_uri = evhttp_request_get_evhttp_uri(m_request)))
                throw HttpRequestException("Failed to get uri.");
        }
    }

    //----------------------------------------------------------------------
    void HttpRequest::initInputBuf() const throw(HttpRequestException) {

        if (m_inputBuf == nullptr) {
            auto *This = const_cast<HttpRequest *>(this);
            if (!(This->m_inputBuf = evhttp_request_get_input_buffer(m_request)))
                throw HttpRequestException("Failed to get input buffer.");
        }
    }

    //----------------------------------------------------------------------
    void HttpRequest::initOutputBuf() const throw(HttpRequestException) {

        if (m_outputBuf == nullptr) {
            auto *This = const_cast<HttpRequest *>(this);
            if (!(This->m_outputBuf = evhttp_request_get_output_buffer(m_request)))
                throw HttpRequestException("Failed to get output buffer.");
        }
    }

    //----------------------------------------------------------------------
    void HttpRequest::initInputHeaders() const throw(HttpRequestException) {

        if (m_inputHeaders == nullptr) {
            auto *This = const_cast<HttpRequest *>(this);
            if (!(This->m_inputHeaders = evhttp_request_get_input_headers(m_request)))
                throw HttpRequestException("Failed to get http input headers.");
        }
    }

    //----------------------------------------------------------------------
    void HttpRequest::initOutputHeaders() const throw(HttpRequestException) {

        if (m_outputHeaders == nullptr) {
            auto *This = const_cast<HttpRequest *>(this);
            if (!(This->m_outputHeaders = evhttp_request_get_output_headers(m_request)))
                throw HttpRequestException("Failed to get http output headers.");
        }
    }

    //----------------------------------------------------------------------
    HttpRequestType HttpRequest::getRequestType() const throw(HttpRequestException) {

        switch (evhttp_request_get_command(m_request)) {
            case EVHTTP_REQ_GET :
                return HttpRequestType::GET;
            case EVHTTP_REQ_POST :
                return HttpRequestType::POST;
            case EVHTTP_REQ_HEAD :
                return HttpRequestType::HEAD;
            case EVHTTP_REQ_PUT :
                return HttpRequestType::PUT;
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
    std::size_t HttpRequest::getInputContentSize() const {

        this->initInputBuf();
        return evbuffer_get_length(m_inputBuf);
    }

    //----------------------------------------------------------------------
    std::size_t HttpRequest::getOutputContentSize() const {

        this->initOutputBuf();
        return evbuffer_get_length(m_outputBuf);
    }

    //----------------------------------------------------------------------
    bool HttpRequest::getInputContent(void *buf, std::size_t len, bool remove) const throw(HttpRequestException) {

        if (len > this->getInputContentSize())
            throw HttpRequestException("Required length of data buffer more than exists.");

        if (remove) {
            if (evbuffer_remove(m_inputBuf, buf, len) == -1)
                throw HttpRequestException("Failed to get input data.");
            return true;
        }

        if (evbuffer_copyout(m_inputBuf, buf, len) == -1)
            throw HttpRequestException("Failed to get input data.");
        return true;
    }

    //----------------------------------------------------------------------
    bool HttpRequest::getOutputContent(void *buf, std::size_t len, bool remove) const throw(HttpRequestException) {

        if (len > this->getOutputContentSize())
            throw HttpRequestException("Required length of data buffer more than exists.");

        if (remove) {
            if (evbuffer_remove(m_outputBuf, buf, len) == -1)
                throw HttpRequestException("Failed to get output data.");
            return true;
        }

        if (evbuffer_copyout(m_outputBuf, buf, len) == -1)
            throw HttpRequestException("Failed to get output data.");
        return true;
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

        std::stringstream ss;
        ss << query;
        for (std::string s; ss;) {
            std::getline(ss, s, '&');
            auto pos = s.find('=');
            if (pos != std::string::npos)
                params[s.substr(0, pos)] = s.substr(pos + 1);
            else
                params[s] = "";
        }
        return std::move(params);
    }

    //----------------------------------------------------------------------
    void HttpRequest::setResponseAttr(const std::string &name, const std::string &val) throw(HttpRequestException) {

        this->initOutputHeaders();
//        evhttp_remove_header(m_request->output_headers, name.c_str());
        if (evhttp_add_header(m_request->output_headers, name.c_str(), val.c_str()) == -1)
            throw HttpRequestException("Failed to set response header attribute.");
    }

    //----------------------------------------------------------------------
    void HttpRequest::setResponseCode(int code) {
        m_responseCode = code;
    }

    //----------------------------------------------------------------------
    void HttpRequest::setResponseString(const std::string &str) throw(HttpRequestException) {

        this->initOutputBuf();
        if (evbuffer_add_printf(m_outputBuf, str.c_str(), "") == -1)
            throw HttpRequestException("Failed to make response.");
    }

    //----------------------------------------------------------------------
    void HttpRequest::setResponseBuf(const void *data, std::size_t bytes) throw(HttpRequestException) {

        this->initOutputBuf();
        void *newData = ::operator new(bytes);
        std::memcpy(newData, data, bytes);
        if (evbuffer_add_reference(m_outputBuf, newData, bytes, &HttpRequest::freeBuf, nullptr) == -1) {
            ::operator delete(newData);
            throw HttpRequestException("Failed to make response.");
        }
    }

    //----------------------------------------------------------------------
    void HttpRequest::setResponseFile(const std::string &fileName) throw(HttpRequestException) {

        // Try to load file from cache
        const AppConfig* config = AppServices::getInstance().getService<AppConfig>();

        if (config->isCachingEnabled()) {
            const CacheService* cache = AppServices::getInstance().getService<CacheService>();
            char* cachedValue = NULL;
            size_t cachedValLen = 0;
            if ((cachedValue = cache->load(fileName.c_str(), fileName.size(), &cachedValLen)) != NULL) {
                this->setResponseBuf(cachedValue, cachedValLen);
                free(cachedValue);
                return;
            }
        }


        // File is no in the cache, load from disk and store in the cache
        auto fileDeleterFunct = [](int *f) {
            if (*f != -1) close(*f);
            delete f;
        };

        std::unique_ptr<int, decltype(fileDeleterFunct)> file(new int(open(fileName.c_str(), O_RDONLY)),
                                                              fileDeleterFunct);
        if (*file == -1)
            throw HttpRequestException(HTTP_NOTFOUND, "Could not find content for uri.");

        ev_off_t length = lseek(*file, 0, SEEK_END);
        if (length == -1 || lseek(*file, 0, SEEK_SET) == -1)
            throw HttpRequestException("Failed to calc file size.");

        this->initOutputBuf();
        // Send file
        if (evbuffer_add_file(m_outputBuf, *file, 0, length) == -1)
            throw HttpRequestException("Failed to make response.");

        *file.get() = -1;

        if (config->isCachingEnabled()) {
            // Store in the cache
            const CacheService* cache = AppServices::getInstance().getService<CacheService>();
            char* cachedValue = (char *) malloc(length);
            if(this->getOutputContent(cachedValue, length, false) != -1) {
                cache->store(fileName.c_str(), fileName.size(), cachedValue, length);
            }
            free(cachedValue);
        }
    }
}
