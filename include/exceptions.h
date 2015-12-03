#ifndef __NETWORK_EXCEPTIONS_H__
#define __NETWORK_EXCEPTIONS_H__

#include <stdexcept>

namespace Network {

    class HttpRuntimeException : public std::runtime_error {
    public:
        HttpRuntimeException(const std::string &arg)
                : std::runtime_error(arg) { }

        HttpRuntimeException(int _code, const std::string &arg)
                : std::runtime_error(arg), code(_code) { }

        virtual int getCode() const {
            return code;
        }

    private:
        int code = 0;
    };

    class HttpRequestException : public HttpRuntimeException {
    public:
        HttpRequestException(const std::string &arg)
                : HttpRuntimeException(arg) { }

        HttpRequestException(int _code, const std::string &arg)
                : HttpRuntimeException(_code, arg) { }
    };

    class HttpServerException : public HttpRuntimeException {
    public:
        HttpServerException(const std::string &arg)
                : HttpRuntimeException(arg) { }

        HttpServerException(int _code, const std::string &arg)
                : HttpRuntimeException(_code, arg) { }
    };

}

#endif  // !__NETWORK_EXCEPTIONS_H__
