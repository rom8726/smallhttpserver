#include "http_server.h"

#include <chrono>
#include <sstream>
#include <cstring>

#include <evhttp.h>
#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#define open _open
#define lseek _lseek
#define close _close
#else

#include <unistd.h>

#define open open
#define lseek lseek
#define close close
#endif

namespace Network {

    namespace {
        class HttpRequest final
                : private Common::NonCopyable, public IHttpRequest
        {
        public:
            HttpRequest(evhttp_request* request)
                    : m_request(request) {
            }

            int getResponseCode() const {
                return m_responseCode;
            }

        private:
            evhttp_request* m_request;
            evkeyvalq* m_inputHeaders = nullptr;
            evkeyvalq* m_outputHeaders = nullptr;
            evbuffer* m_inputBuf = nullptr;
            evbuffer* m_outputBuf = nullptr;
            const evhttp_uri* m_uri = nullptr;
            int m_responseCode = HTTP_OK;

            void initUri() const {
                if (m_uri)
                    return;
                auto *This = const_cast<HttpRequest *>(this);
                if (!(This->m_uri = evhttp_request_get_evhttp_uri(m_request)))
                    throw HttpRequestException("Failed to get uri.");
            }

            void initOutputBuf() const {
                if (m_outputBuf)
                    return;
                auto *This = const_cast<HttpRequest *>(this);
                if (!(This->m_outputBuf = evhttp_request_get_output_buffer(m_request)))
                    throw HttpRequestException("Failed to get output buffer.");
            }

            void initInputHeaders() const {
                if (m_inputHeaders != nullptr)
                    return;
                auto *This = const_cast<HttpRequest *>(this);
                if (!(This->m_inputHeaders = evhttp_request_get_input_headers(m_request)))
                    throw HttpRequestException("Failed to get http input headers.");
            }

            void initOutputHeaders() const {
                if (m_outputHeaders != nullptr)
                    return;
                auto *This = const_cast<HttpRequest *>(this);
                if (!(This->m_outputHeaders = evhttp_request_get_output_headers(m_request)))
                    throw HttpRequestException("Failed to get http output headers.");
            }

            static void freeBuf(const void* data, std::size_t datalen, void* extra) {
                ::operator delete(const_cast<void *>(data));
            }

            virtual Type getRequestType() const {
                switch (evhttp_request_get_command(m_request)) {
                    case EVHTTP_REQ_GET :
                        return Type::GET;
                    case EVHTTP_REQ_POST :
                        return Type::POST;
                    case EVHTTP_REQ_HEAD :
                        return Type::HEAD;
                    case EVHTTP_REQ_PUT :
                        return Type::PUT;
                    default :
                        break;
                }
                throw HttpRequestException("Unknown request type.");
            }

            virtual const std::string getHeaderAttr(const char* attrName) const {
                this->initInputHeaders();
                const char* ret = evhttp_find_header(m_inputHeaders, attrName);
                return ret ? ret : "";
            }

            virtual std::size_t getContentSize() const {
                if (m_inputBuf == nullptr) {
                    auto *This = const_cast<HttpRequest *>(this);
                    if (!(This->m_inputBuf = evhttp_request_get_input_buffer(m_request)))
                        throw HttpRequestException("Failed to get input buffer.");
                }
                return evbuffer_get_length(m_inputBuf);
            }

            virtual void getContent(void* buf, std::size_t len, bool remove) const {
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

            virtual const std::string getPath() const {
                this->initUri();
                const char* ret = evhttp_uri_get_path(m_uri);
                return ret ? ret : "";
            }

            virtual const RequestParams getParams() const {
                this->initUri();
                RequestParams params;
                const char* query = evhttp_uri_get_query(m_uri);
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

            virtual void setResponseAttr(const std::string& name, const std::string& val) {
                this->initOutputHeaders();
                if (evhttp_add_header(m_outputHeaders, name.c_str(), val.c_str()) == -1)
                    throw HttpRequestException("Failed to set response header attribute.");
            }

            virtual void setResponseCode(int code) {
                m_responseCode = code;
            }

            virtual void setResponseString(const std::string& str) {
                this->initOutputBuf();
                if (evbuffer_add_printf(m_outputBuf, str.c_str()) == -1)
                    throw HttpRequestException("Failed to make response.");
            }

            virtual void setResponseBuf(const void* data, std::size_t bytes) {
                this->initOutputBuf();
                void* newData = ::operator new(bytes);
                std::memcpy(newData, data, bytes);
                if (evbuffer_add_reference(m_outputBuf, newData, bytes, &HttpRequest::freeBuf, nullptr) == -1) {
                    ::operator delete(newData);
                    throw HttpRequestException("Failed to make response.");
                }
            }

            virtual void setResponseFile(const std::string& fileName) {
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
        };

        struct RequestParams {
            HttpServer::OnRequestFunc func;
            volatile bool* isInProcess = nullptr;
        };

        void onRawRequest(evhttp_request* request, void* prm) {
            try {
                auto pRequest = std::make_shared<HttpRequest>(request);
                auto *reqPrm = reinterpret_cast<RequestParams *>(prm);

                Common::BoolFlagInvertor flagInvertor(reqPrm->isInProcess);
                reqPrm->func(pRequest);

                auto *outputBuffer = evhttp_request_get_output_buffer(request);
                if (!outputBuffer)
                    throw HttpRequestException("Failed to get output buffer.");

                evhttp_send_reply(request, pRequest->getResponseCode(), "", outputBuffer);
            }
            catch (const HttpRequestException& e) {
                if (e.getCode()) {
                    std::stringstream ioStream;
                    ioStream << "<html><body>"
                            "<hr/><center><h1>"
                    << e.getCode()
                    << ". "
                    << e.what()
                    << "</center><hr/></h1>"
                    << "</body></html>";
                    evhttp_send_error(request, e.getCode(), ioStream.str().c_str());
                } else {
                    
                    evhttp_send_error(request, HTTP_INTERNAL,
                                      "<html><body>"
                                              "<hr/><center><h1>500. Internal error.</center><hr/></h1>"
                                              "</body></html>");
                }
            } catch (const std::exception& e) {
                evhttp_send_error(request, HTTP_INTERNAL,
                                  "<html><body>"
                                          "<hr/><center><h1>500. Internal error.</center><hr/></h1>"
                                          "</body></html>");
            }
        }

        int httpRequestTypeToAllowedMethod(const IHttpRequest::Type &type) {
            switch (type) {
                case IHttpRequest::Type::GET :
                    return EVHTTP_REQ_GET;
                case IHttpRequest::Type::HEAD :
                    return EVHTTP_REQ_HEAD;
                case IHttpRequest::Type::PUT :
                    return EVHTTP_REQ_PUT;
                case IHttpRequest::Type::POST :
                    return EVHTTP_REQ_POST;
                default :
                    break;
            }
            throw HttpRequestException("Method not allowed.");
        }

    }

    HttpServer::HttpServer(const std::string& address, std::uint16_t port,
                           std::uint16_t threadCount, const OnRequestFunc& onRequest,
                           const MethodPool& allowedMethodsArg,
                           std::size_t maxHeadersSize, std::size_t maxBodySize)
            : m_isRunInvertor(&m_isRun)
    {
        int allowedMethods = -1;
        for (const auto i : allowedMethodsArg)
            allowedMethods |= httpRequestTypeToAllowedMethod(i);

        volatile bool doneInitThread = false;
        std::exception_ptr except;
        evutil_socket_t sock = -1;
        
        auto threadFunc = [&]() {
            try {
                volatile bool processRequest = false;
                RequestParams reqPrm;
                reqPrm.func = onRequest;
                reqPrm.isInProcess = &processRequest;

                typedef std::unique_ptr<event_base, decltype(&event_base_free)> EventBasePtr;
                EventBasePtr eventBasePtr(event_base_new(), &event_base_free);

                if (!eventBasePtr)
                    throw HttpServerException("Failed to create new base_event.");

                typedef std::unique_ptr<evhttp, decltype(&evhttp_free)> EvHttpPtr;
                EvHttpPtr evHttpPtr(evhttp_new(eventBasePtr.get()), &evhttp_free);

                if (!evHttpPtr)
                    throw HttpServerException("Failed to create new evhttp.");

                evhttp_set_allowed_methods(evHttpPtr.get(), allowedMethods);

                if (maxHeadersSize != MaxHeaderSize)
                    evhttp_set_max_headers_size(evHttpPtr.get(), maxHeadersSize);
                if (maxBodySize != MaxBodySize)
                    evhttp_set_max_body_size(evHttpPtr.get(), maxBodySize);

                evhttp_set_gencb(evHttpPtr.get(), &onRawRequest, &reqPrm);

                if (sock == -1) {
                    auto* boundSock = evhttp_bind_socket_with_handle(evHttpPtr.get(), address.c_str(), port);
                    if (!boundSock)
                        throw HttpServerException("Failed to bind server socket.");
                    if ((sock = evhttp_bound_socket_get_fd(boundSock)) == -1)
                        throw HttpServerException("Failed to get server socket for next instance.");
                } else {
                    if (evhttp_accept_socket(evHttpPtr.get(), sock) == -1)
                        throw HttpServerException("Failed to bind server socket for new instance.");
                }

                doneInitThread = true;

                while(m_isRun) {
                    processRequest = false;
                    if (event_base_loop(eventBasePtr.get(), EVLOOP_NONBLOCK) == -1) {
                        // TODO: to log
                        //std::cerr << "Loop error." << std::endl;
                    }
                    if (!processRequest)
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            catch (...) {
                except = std::current_exception();
            }
        };


        ThreadsPool threadsPool;
        for (int i = 0; i < threadCount; ++i) {
            doneInitThread = false;

            ThreadPtr threadPtr(new std::thread(threadFunc), threadDeleterFunct);
            threadsPool.push_back(std::move(threadPtr));

            for (;;) {
                if (except != std::exception_ptr()) {
                    m_isRun = false;
                    std::rethrow_exception(except);
                }
                if (doneInitThread)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        m_threadsPool = std::move(threadsPool);
    }

}
