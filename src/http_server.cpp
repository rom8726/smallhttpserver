#include "http_server.h"
#include "http_request.h"

#include <sstream>
#include <cstring>


namespace Network {

    //----------------------------------------------------------------------
    struct RawRequestCallbackParams {
        HttpServer::OnRequestFunc func;
        volatile bool *isInProcess = nullptr;
    };

    //----------------------------------------------------------------------
    void onRawRequest(evhttp_request *request, void *prm) {
        try {
            auto pRequest = std::make_shared<HttpRequest>(request);
            auto *reqPrm = reinterpret_cast<RawRequestCallbackParams *>(prm);

            Common::BoolFlagInvertor flagInvertor(reqPrm->isInProcess);
            reqPrm->func(pRequest);

            auto *outputBuffer = evhttp_request_get_output_buffer(request);
            if (!outputBuffer)
                throw HttpRequestException("Failed to get output buffer.");

            evhttp_send_reply(request, pRequest->getResponseCode(), "", outputBuffer);

        } catch (const HttpRequestException &e) {
            if (e.getCode()) {
                std::stringstream ioStream;
                ioStream << "<html><body>"
                << "<hr/><center><h1>"
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

        } catch (const std::exception &e) {
            evhttp_send_error(request, HTTP_INTERNAL,
                              "<html><body>"
                                      "<hr/><center><h1>500. Internal error.</center><hr/></h1>"
                                      "</body></html>");
        }
    }

    //----------------------------------------------------------------------
    int httpRequestTypeToAllowedMethod(const HttpRequestType &type) {
        switch (type) {
            case HttpRequestType::GET :
                return EVHTTP_REQ_GET;
            case HttpRequestType::HEAD :
                return EVHTTP_REQ_HEAD;
            case HttpRequestType::PUT :
                return EVHTTP_REQ_PUT;
            case HttpRequestType::POST :
                return EVHTTP_REQ_POST;
            default :
                break;
        }
        throw HttpRequestException("Method not allowed.");
    }

    //----------------------------------------------------------------------
    //----------------------------------------------------------------------
    HttpServer::HttpServer(const std::string &address, std::uint16_t port,
                           std::uint16_t threadCount, const OnRequestFunc &onRequest,
                           const MethodPool &allowedMethodsArg,
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
                RawRequestCallbackParams reqPrm;
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
                    auto *boundSock = evhttp_bind_socket_with_handle(evHttpPtr.get(), address.c_str(), port);
                    if (!boundSock)
                        throw HttpServerException("Failed to bind server socket.");
                    if ((sock = evhttp_bound_socket_get_fd(boundSock)) == -1)
                        throw HttpServerException("Failed to get server socket for next instance.");
                } else {
                    if (evhttp_accept_socket(evHttpPtr.get(), sock) == -1)
                        throw HttpServerException("Failed to bind server socket for new instance.");
                }

                doneInitThread = true;

                while (m_isRun) {
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

            for (; ;) {
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
