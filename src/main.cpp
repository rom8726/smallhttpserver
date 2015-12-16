#include "http_server.h"
#include "demonizer.h"
#include "proj_defs.h"

#include <iostream>
#include <null_logger.h>
#include <syslog_logger.h>

#if USE_BOOST_FILEPATH
#include <boost/filesystem.hpp>
#else
#ifdef __linux__
#include <linux/limits.h>
#else
#define PATH_MAX 4096
#endif
#endif

using namespace System;
using namespace Common;
using namespace Common::Services;


//----------------------------------------------------------------------
int workFunc();
int workStopFunc();
int workRereadConfigFunc();
std::string getConfigPath();
void initServices(bool isDaemon, const char* pathToConfig = NULL);

static std::shared_ptr<Network::HttpServer> server(nullptr);
static std::string gPathToConfig = getConfigPath();

//----------------------------------------------------------------------
int main(int argc, char* argv[]) {

    const std::string daemonName = "smallhttpserver";
    bool isDaemon = true;

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

    initServices(isDaemon);
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

                          req->setResponseAttr("Server", "smallhttpserver");
                          req->setResponseAttr("Content-Type", config->getContentTypeFromFileName(path));
                          req->setResponseFile(path);

                          if (config->isLogging()) {
                              std::stringstream ss;
                              ss << "path: " << path << std::endl
                              << "host: "
                              << req->getHeaderAttr("host") << std::endl
                              << "referer: "
                              << req->getHeaderAttr("referer") << std::endl;

                              logger->log(ss.str());
                          }
                      }
        ));

        if (config->isDaemon()) {
            // TODO:
            while (!server->isAllThreadsDone()) {
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
    while (!server->isAllThreadsDone()) {
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

//----------------------------------------------------------------------
void initServices(bool isDaemon, const char* _pathToConfig) {
    AppServices& services = AppServices::getInstance();
    LoggerPtr& logger = AppServices::getLogger();

    services.clear();

    std::string pathToConfig;
    if (_pathToConfig == NULL) {
        pathToConfig = gPathToConfig;
    } else {
        pathToConfig = std::string(_pathToConfig);
    }

    ServicePtr iConfig(new AppConfig(pathToConfig));
    AppConfig* config = static_cast<AppConfig*>(iConfig.get());

    logger->log("Config reading...");
    if (config->init() == false) {
        logger->err("init app config failed!");
        exit(EXIT_FAILURE);
    }
    config->setDaemon(isDaemon);
    services.addService<AppConfig>(iConfig);
    logger->log("Config reading : SUCCESS");

    if (config->isCachingEnabled()) {
        ServicePtr iCache(new CacheService(config->getMemcachedServerPort()));
        if (iCache->init() == false) {
            logger->err("init cache service failed!");
            exit(EXIT_FAILURE);
        }

        services.addService<CacheService>(iCache);
    }
}

//----------------------------------------------------------------------
std::string getConfigPath() {
#if USE_BOOST_FILEPATH
    return boost::filesystem::current_path().string() + "/config.json";
#else
    char pathbuf[PATH_MAX];
    char *pathres = realpath("./config.json", pathbuf);
    if (!pathres) {
        AppServices::getLogger()->err("realpath for config.json failed!");
        free(pathres);
        exit(EXIT_FAILURE);
    }
    return std::string(pathres);
#endif
}