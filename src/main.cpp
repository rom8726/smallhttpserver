#include "http_server.h"
#include "http_headers.h"
#include "http_content_type.h"
#include "app_services.h"
#include "demonizer.h"

#include <iostream>
#include <null_logger.h>
#include <syslog_logger.h>

#ifdef __linux__
#include <linux/limits.h>
#else
#define PATH_MAX 4096
#endif

using namespace System;
using namespace Common;
using namespace Common::Services;


//----------------------------------------------------------------------
static std::shared_ptr<Network::HttpServer> server(nullptr);
static std::string gPathToConfig;

void initServices(bool isDaemon, const char* pathToConfig = NULL);
int workFunc();
int workStopFunc();
int workRereadConfigFunc();

//----------------------------------------------------------------------
int main(int argc, char* argv[]) {

    const std::string daemonName = "smallhttpserver";
    bool isDaemon = true;

//    signal(SIGPIPE, SIG_IGN); //ignore SIGPIPE for send() error recovery

    if (argc == 2) {

        if (!strcmp(argv[1], "console")) {
            isDaemon = false;

        } else if (!strcmp(argv[1], "stop")) {
            ConsoleLogger consoleLogger;
            consoleLogger.log("Stopping daemon...");

            std::unique_ptr<Demonizer> daemon(new Demonizer);
            daemon->setName(daemonName);
            try {
                daemon->stopWorker();
            } catch (const SystemException& ex) {
                consoleLogger.err("Stop exception :" + std::string(ex.what()));
                exit(1);
            }
            consoleLogger.log("Stop OK");
            exit(0);

        } else if (!strcmp(argv[1], "reconfig")) {

            ConsoleLogger consoleLogger;
            consoleLogger.log("Sending reconfig signal to daemon...");

            std::unique_ptr<Demonizer> daemon(new Demonizer);
            daemon->setName(daemonName);
            try {
                daemon->sendUserSignalToWorker();
            } catch (const SystemException& ex) {
                consoleLogger.err("send signal exception :" + std::string(ex.what()));
                exit(1);
            }
            consoleLogger.log("Send OK");
            exit(0);
        }
    }

    char pathbuf[PATH_MAX];
    char *pathres = realpath("./config.json", pathbuf);
    if (!pathres) {
        AppServices::getLogger()->err("realpath for config.json failed!");
        free(pathres);
        exit(EXIT_FAILURE);
    }
    gPathToConfig = std::string(pathres);

    initServices(isDaemon, gPathToConfig.c_str());
    AppServices& services = AppServices::getInstance();
    AppConfig* config = services.getService<AppConfig>();

    if (!config->isLogging()) {// log off
        AppServices::setLogger(LoggerPtr(new NullLogger));
    }

    if (isDaemon) {
        //demonize

        //config->setDaemon(true);
        std::cout << "Switch to daemon state now (see syslog for output)..." << std::endl;

        if (config->isLogging()) {
            AppServices::setLogger(LoggerPtr(new SyslogLogger()));
        }

        LoggerPtr& logger = AppServices::getLogger();

        std::unique_ptr<Demonizer> daemon(new Demonizer);
        daemon->setName(daemonName);
        logger->setName(daemonName);
        logger->log("Setup daemon...");

        try {
            daemon->setup();
        } catch (const SystemException& ex) {
            logger->err(ex.what());
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
void initServices(bool isDaemon, const char* _pathToConfig) {
    AppServices& services = AppServices::getInstance();
    services.clear();

    std::string pathToConfig;
    if (_pathToConfig == NULL) {
        pathToConfig = gPathToConfig;
    } else {
        pathToConfig = std::string(_pathToConfig);
    }

    ServicePtr iConfig(new AppConfig(pathToConfig));
    AppConfig* config = static_cast<AppConfig*>(iConfig.get());
    LoggerPtr& logger = AppServices::getLogger();

    logger->log("Config reading...");
    if (config->init() == false) {
        logger->err("init app config failed!");
        exit(EXIT_FAILURE);
    }
    config->setDaemon(isDaemon);
    services.addService<AppConfig>(iConfig);
    logger->log("Config reading : SUCCESS");

    if (config->isCachingEnabled()) {
        ServicePtr iCache(new CacheService);
        CacheService* cache = static_cast<CacheService*>(iCache.get());
        if (cache->init(config->getMemcachedServerPort()) == false) {
            logger->err("init cache service failed!");
            exit(EXIT_FAILURE);
        }

        services.addService<CacheService>(iCache);
    }
}

//----------------------------------------------------------------------
int workFunc() {

    AppServices& services = AppServices::getInstance();
    AppConfig* config = services.getService<AppConfig>();
    LoggerPtr& logger = AppServices::getLogger();
    logger->log("workFunc start");

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

                              logger->log(ss.str());
                          }
                      }
        ));

        if (config->isDaemon()) {
            // TODO:
            while (server->isRun()) {
                std::this_thread::yield();
            }
        } else {
            std::cin.get();
            server->stop();
        }
    }
    catch (std::exception const &e) {
        logger->err(e.what());
        exit(EXIT_FAILURE);
        //return EXIT_FAILURE;
    }

    exit(EXIT_SUCCESS);
    //return EXIT_SUCCESS;
}

//----------------------------------------------------------------------
int workStopFunc() {
    LoggerPtr& logger = AppServices::getLogger();
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

    AppServices::setLogger(LoggerPtr(new SyslogLogger));

    initServices(true);

    AppServices& services = AppServices::getInstance();
    AppConfig* config = services.getService<AppConfig>();
    if (!config->isLogging()) {
        AppServices::setLogger(LoggerPtr(new NullLogger));
    }

    workFunc();

    return EXIT_SUCCESS;
}
