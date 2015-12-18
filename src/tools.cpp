#include "tools.h"

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

namespace System {

    //----------------------------------------------------------------------
    std::string SystemTools::calculateFilenameToStorePID(const std::string &processName) {
        return "/var/run/" + processName + ".pid";
    }

    //----------------------------------------------------------------------
    bool SystemTools::checkRunningAndSavePID(const std::string &processName) throw(SystemException) {
        //get name
        std::string pidFile = calculateFilenameToStorePID(processName);
        //try to open file
        int fd = open(pidFile.c_str(), O_RDWR | O_CREAT,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        if (fd < 0) {
            throw SystemException("can`t open PID file " + pidFile);
        }

        //try to lock it
        if (lockf(fd, F_TLOCK, 0)) {
            if (errno == EACCES || errno == EAGAIN) {
                close(fd);
                return 1;// already locked - by another process instance
            }
            throw SystemException("can`t lock PID file");
        }

        ssize_t r = 0;
        r = ftruncate(fd, 0);
        char buf[255];
        sprintf(buf, "%d", (int) getpid());
        r = write(fd, buf, strlen(buf));
        return (r == -1) ? false : true;
    }

    //----------------------------------------------------------------------
    bool SystemTools::checkRunningAndSavePID(const std::string &processName, int pid) throw(SystemException) {
        //get name
        std::string pidFile = calculateFilenameToStorePID(processName);
        //try to open file
        int fd = open(pidFile.c_str(), O_RDWR | O_CREAT,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        if (fd < 0) {
            throw SystemException("can`t open PID file " + pidFile);
        }

        //try to lock it
        if (lockf(fd, F_TLOCK, 0)) {
            if (errno == EACCES || errno == EAGAIN) {
                close(fd);
                return 1;// already locked - by another process instance
            }
            throw SystemException("can`t lock PID file");
        }

        ssize_t r = 0;
        r = ftruncate(fd, 0);
        char buf[255];
        sprintf(buf, "%d", pid);
        r = write(fd, buf, strlen(buf));
        return (r == -1) ? false : true;
    }

    //----------------------------------------------------------------------
    int SystemTools::findPID(const std::string &processName) throw(SystemException) {
        std::string pidFile = calculateFilenameToStorePID(processName);
        //read pid from file
        int fd = open(pidFile.c_str(), O_RDWR | O_CREAT,
                      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd < 0) {
            throw SystemException("can`t open PID file");
        }

        char buf[255];
        memset(buf, 0, sizeof(buf));
        if (read(fd, buf, sizeof(buf)) <= 0)
            return -1;
        close(fd);

        return atoi(buf);
    }

    //----------------------------------------------------------------------
    void SystemTools::kill(int pid) {
        ::kill(pid, SIGTERM);
    }

    //----------------------------------------------------------------------
    void SystemTools::killWithSignal(int pid, int sig) {
        ::kill(pid, sig);
    }
}
