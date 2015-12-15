#include "http_server.h"
#include "http_headers.h"
#include "http_content_type.h"
#include "app_services_factory.h"
#include "demonizer.h"

#include <memory>
#include <iostream>
#include <null_logger.h>
#include <console_logger.h>
#include <syslog_logger.h>
#include <unistd.h>

using namespace System;
using namespace Common;
using namespace Common::Services;


//----------------------------------------------------------------------
static std::shared_ptr<Logger> logger(new ConsoleLogger);
static std::shared_ptr<Network::HttpServer> server(nullptr);
static bool isDaemon = true;

int workFunc();
int workStopFunc();
int workRereadConfigFunc();

//----------------------------------------------------------------------
int main(int argc, char* argv[]) {

    std::string daemonName = "smallhttpserver";

//    signal(SIGPIPE, SIG_IGN); //ignore SIGPIPE for send() error recovery

    if (argc == 2) {

        if (!strcmp(argv[1], "console"))
            isDaemon = false;

        if (!strcmp(argv[1], "stop")) {
            std::cout << "Stopping daemon..." << std::endl;
            std::unique_ptr<Demonizer> daemon(new Demonizer);
            daemon->setName(daemonName);
            try {
                daemon->stopWorker();
            } catch (const SystemException& ex) {
                std::cout << "Stop exception :" << ex.what() << std::endl;
                exit(1);
            }
            std::cout << "Stop ok" << std::endl;
            exit(0);
        }
        if (!strcmp(argv[1], "reconfig")) {

            std::cout << "Sending reconfig signal to daemon..." << std::endl;
            std::unique_ptr<Demonizer> daemon(new Demonizer);
            daemon->setName(daemonName);
            try {
                daemon->sendUserSignalToWorker();
            } catch (const SystemException& ex) {
                std::cout << "send signal exception :" << ex.what() << std::endl;
                exit(1);
            }
            std::cout << "Send ok" << std::endl;
            exit(0);
        }
    }

    std::cout << "Config reading..." << std::endl;
    AppServicesFactory& services = AppServicesFactory::getInstance();
    if (!services.initAll()) {
        std::cerr << "Couldn't init app services!" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Config reading : SUCCESS" << std::endl;

    AppConfig* config = services.getService<AppConfig>();
    config->setDaemon(false);

    if (!config->isLogging()) {// log off
        logger.reset(new NullLogger);
        config->setLogger(logger);
    }

    if (isDaemon) {
        //demonize

        config->setDaemon(true);
        logger->log("Switch to daemon state now (see syslog for output)...");

        if (config->isLogging()) {
            logger.reset(new SyslogLogger());
            config->setLogger(logger);
        }

        std::unique_ptr<Demonizer> daemon(new Demonizer);
        daemon->setName(daemonName);
        logger->setName(daemonName);
        logger->log("Setup daemon...");

        try {
            daemon->setup();
        } catch (const SystemException& ex) {
            logger->log(ex.what());
            exit(1);
        }

        logger->log("Start with monitoring...");
        daemon->startWithMonitoring(workFunc, workStopFunc, workRereadConfigFunc);
    } else {
        //no daemon
        workFunc();
    }

    return 0;
}

//----------------------------------------------------------------------
int workFunc() {

    AppServicesFactory& services = AppServicesFactory::getInstance();
    AppConfig* config = services.getService<AppConfig>();
    config->getLogger()->log("workFunc start");

    try {
        using namespace Network;
        server.reset(new HttpServer(config->getServerIp(), config->getServerPort(), config->getThreadsCnt(),

                      [&](IHttpRequestPtr req) {
                          std::string path = req->getPath();
                          path = config->getRootDir() + path + (path == "/" ? config->getDefaultPage() : std::string());

                          req->setResponseAttr(Http::Response::Header::Server::Value, "MyTestServer");
                          req->setResponseAttr(Http::Response::Header::ContentType::Value,
                                               Http::Content::TypeFromFileName(path));
                          req->setResponseFile(path);

                          if (config->isLogging()) {
                              std::stringstream ss;
                              ss << "path: " << path << std::endl
                              << Http::Request::Header::Host::Name << ": "
                              << req->getHeaderAttr(Http::Request::Header::Host::Value) << std::endl
                              << Http::Request::Header::Referer::Name << ": "
                              << req->getHeaderAttr(Http::Request::Header::Referer::Value) << std::endl;

                              config->getLogger()->log(ss.str());
                          }
                      }
        ));

        if (config->isDaemon()) {
            while (server->isRun()) {
                std::this_thread::yield();
            }
        } else {
            std::cin.get();
            server->stop();
        }
    }
    catch (std::exception const &e) {
        logger->log(e.what());
        exit(EXIT_FAILURE);
        return EXIT_FAILURE;
    }

    exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}

//----------------------------------------------------------------------
int workStopFunc() {
    logger->log("Stopping server...");

    server->stop();
    while (server->isRun()) {
        logger->log(".");
        std::this_thread::yield();
    }

    server.reset();

    logger->log("Server stopped successfully!");
    return EXIT_SUCCESS;
}

//----------------------------------------------------------------------
int workRereadConfigFunc() {

    workStopFunc();

    logger->log("Config reading...");
    AppServicesFactory& services = AppServicesFactory::getInstance();
    if (!services.initAll()) {
        logger->log("Couldn't init app services!");
        exit(EXIT_FAILURE);
        return EXIT_FAILURE;
    }
    AppConfig* config = services.getService<AppConfig>();
    config->setDaemon(true);

    logger->log("Config reading : SUCCESS");

    workFunc();

    return EXIT_SUCCESS;
}
