#include "http_server.h"
#include "http_headers.h"
#include "http_content_type.h"
#include "app_config.h"

#include <sstream>
#include <iostream>
//#include <unistd.h>

using namespace Common::Config;

int main() {

    AppConfig& config = AppConfig::getInstance();
    if (!config.init()) {
        std::cerr << "Couldn't init config!" << std::endl;
        return EXIT_FAILURE;
    }

//    if (chroot(config.getRootDir().c_str()) == -1) {
//        std::cerr << "chroot failed!" << std::endl;
//        return EXIT_FAILURE;
//    }

    try {
        using namespace Network;
        HttpServer server(config.getServerIp(), config.getServerPort(), config.getThreadsCnt(),

            [&](IHttpRequestPtr req) {
                std::string path = req->getPath();
                path = config.getRootDir() + path + (path == "/" ? config.getDefaultPage() : std::string());

                req->setResponseAttr(Http::Response::Header::Server::Value, "MyTestServer");
                req->setResponseAttr(Http::Response::Header::ContentType::Value,
                                     Http::Content::TypeFromFileName(path));
                req->setResponseFile(path);

                if (config.getIsDebug()) {
                    std::stringstream ss;
                    ss << "path: " << path << std::endl
                    << Http::Request::Header::Host::Name << ": "
                    << req->getHeaderAttr(Http::Request::Header::Host::Value) << std::endl
                    << Http::Request::Header::Referer::Name << ": "
                    << req->getHeaderAttr(Http::Request::Header::Referer::Value) << std::endl;

                    std::lock_guard<std::mutex> lock(config.getStdOutMutex());
                    std::cout << ss.str() << std::endl;
                }
            }
        );

        std::cin.get();
    }
    catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
