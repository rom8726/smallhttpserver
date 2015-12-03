#include "http_server.h"
#include "http_headers.h"
#include "http_content_type.h"

#include <iostream>
#include <sstream>
#include <mutex>

int main() {

    char const srvAddress[] = "127.0.0.1";
    std::uint16_t srvPort = 5555;
    std::uint16_t srvThreadCount = 4;
//    std::string const rootDir = "../test_content";
    std::string const rootDir = "/home/roman/projects/cpp/learn-network/libevent_test_http_srv/test_content";
    std::string const defaultPage = "index.html";
    std::mutex mtx;

    try {
        using namespace Network;
        HttpServer server(srvAddress, srvPort, srvThreadCount,

            [&](IHttpRequestPtr req) {
                std::string path = req->getPath();
                path = rootDir + path + (path == "/" ? defaultPage : std::string());

                req->setResponseAttr(Http::Response::Header::Server::Value, "MyTestServer");
                req->setResponseAttr(Http::Response::Header::ContentType::Value,
                                     Http::Content::TypeFromFileName(path));
                req->setResponseFile(path);

                {
                    std::stringstream ss;
                    ss << "path: " << path << std::endl
                    << Http::Request::Header::Host::Name << ": "
                    << req->getHeaderAttr(Http::Request::Header::Host::Value) << std::endl
                    << Http::Request::Header::Referer::Name << ": "
                    << req->getHeaderAttr(Http::Request::Header::Referer::Value) << std::endl;

                    std::lock_guard<std::mutex> lock(mtx);
                    std::cout << ss.str() << std::endl;
                }
            }
        );

        std::cin.get();
    }
    catch (std::exception const &e) {
        std::cout << e.what() << std::endl;
    }

    return 0;
}
