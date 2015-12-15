#include "demonizer.h"
#include "syslog_logger.h"
#include "exceptions.h"

#include <unistd.h>
//#include <pthread.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
//#include <errno.h>
//#include <syslog.h>
#include <fcntl.h>
//#include <signal.h>
//#include <string.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <execinfo.h>
#include <wait.h>

using namespace std;
using namespace Common;

namespace System {

    int (*Demonizer::m_sStartFunc)() = NULL;

    int (*Demonizer::m_sStopFunc)() = NULL;

    int (*Demonizer::m_sRereadCfgFun)() = NULL;
    
    Logger *Demonizer::m_sSysLogger = NULL;

    Demonizer::Demonizer() {
    }

    Demonizer::~Demonizer() {
    }

    void Demonizer::signal_handler(int sig, siginfo_t *si, void *ptr) {

        void *errorAddr;
        void *trace[16];
        int x;
        int traceSize;
        char **messages;
        (void) si;

        m_sSysLogger->log("Caught signal:" + string(strsignal(sig)));

        if (sig == SIGUSR1) {
            m_sSysLogger->log("Received user signal.");
            if (m_sRereadCfgFun != NULL)
                (*m_sRereadCfgFun)();

            return;
        }


        if (sig == SIGTERM) {
            m_sSysLogger->log("Received sigterm signal. Stopping...");
            if (m_sStopFunc != NULL)
                (*m_sStopFunc)();
            exit(CHILD_NEED_TERMINATE);
        }

        //found error address
        #if __WORDSIZE == 64
        errorAddr = (void *) ((ucontext_t *) ptr)->uc_mcontext.gregs[REG_RIP];
        #else
        errorAddr = (void*) ((ucontext_t*) ptr)->uc_mcontext.gregs[REG_EIP];
        #endif
        //backtrace
        traceSize = backtrace(trace, 16);
        trace[1] = errorAddr;
        //know more
        messages = backtrace_symbols(trace, traceSize);
        if (messages) {
            m_sSysLogger->log("== Backtrace ==");
            for (x = 1; x < traceSize; x++) {
                m_sSysLogger->log(messages[x]);
            }
            m_sSysLogger->log("== End Backtrace ==");
            free(messages);
        }
        m_sSysLogger->log("Stopped");

        if (m_sStopFunc != NULL)
            (*m_sStopFunc)();
        //we need child restart
        exit(CHILD_NEED_RESTART);
    }


    /*
     * Demonize the current process.
     */
    void Demonizer::setup() throw(DemonizerException) {

        m_sSysLogger = new SyslogLogger();
        m_sSysLogger->setName(logger->getName());
        m_sSysLogger->log("Configuring daemon...");

        pid_t pid;
        struct rlimit limits;
        struct sigaction sa;

        umask(0);

        if (getrlimit(RLIMIT_NOFILE, &limits) < 0) {
            throw DemonizerException("can`t get RLIMIT_NOFILE");
        }

        //fork process and disconnect it from parent
        if ((pid = fork()) < 0) {
            throw DemonizerException("fork error");

        } else if (0 != pid) {
            exit(0);//stop parent process
        }

        //create seance
        if ((setsid()) == (pid_t) -1) {
            throw DemonizerException("setsig error");

        }
        //ignoring sighup
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(SIGHUP, &sa, NULL) < 0) {
            throw DemonizerException("can`t ignore SIGHUP");
        }

        //fork again
        if ((pid = fork()) < 0) {
            throw DemonizerException("fork error");

        } else if (0 != pid) {
            exit(0);
        }

        //chdir to /
        if (chdir("/") < 0) {
            throw DemonizerException("can`t chdir() to /");
        }
        //close all resources
        if (limits.rlim_max == RLIM_INFINITY)
            limits.rlim_max = 1024;

        u_int32_t idx;
        for (idx = 0; idx < limits.rlim_max; ++idx) {
            close(idx);
        }
        //reopen stdout to /dev/null and another strems to it
        int fd0 = open("/dev/null", O_RDWR);
        int fd1 = dup(0);
        int fd2 = dup(0);

        if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
            throw DemonizerException("bad file descriptors");
        }

        m_sSysLogger->log("Prepare to be daemon ok");

        //check the running -  and save new! pid
        if (PlatformFactory::getInstance()->checkRunningAndSavePID(getName())) {
            m_sSysLogger->log("Daemon is already running");
            throw DemonizerException("Error. Daemon is already running");
        }

        //set handler to signal
        //signal(SIGTERM, signal_handler);
        m_sSysLogger->log("Daemonize done");

    }

   /*
    * stops the daemon service
    */
    void Demonizer::stop() throw(DemonizerException) {

        //get pid from pid file of running daemon
        int pid = PlatformFactory::getInstance()->findPID(this->getName());
        //check running
        if (pid == -1) {
            m_sSysLogger->log("Error. Daemon is not running");
            throw DemonizerException("Daemon is not running");

        } else {
            //kill it
            PlatformFactory::getInstance()->kill(pid);
        }
    }

    void Demonizer::stopWorker() throw(DemonizerException) {

        //get pid from pid file of running daemon
        int pid = PlatformFactory::getInstance()->findPID(this->getName() + "_worker");
        //check running
        if (pid == -1) {
            m_sSysLogger->log("Error. Worker daemon is not running");
            throw DemonizerException("Worker daemon is not running");
        } else {
            //kill it
            PlatformFactory::getInstance()->kill(pid);
        }
    }


    void Demonizer::sendUserSignalToWorker() throw(DemonizerException) {

        //get pid from pid file of running daemon
        int pid = PlatformFactory::getInstance()->findPID(this->getName() + "_worker");
        //check running
        if (pid == -1) {
            m_sSysLogger->log("Error. Worker daemon is not running");
            throw DemonizerException("Worker daemon is not running");

        } else {
            //kill it
            PlatformFactory::getInstance()->killWithSignal(pid, SIGUSR1);
        }
    }

    int Demonizer::workProc() {

        struct sigaction sigact;
        sigset_t sigset;
        int signo;
        int status;
        sigact.sa_flags = SA_SIGINFO;
        sigact.sa_sigaction = signal_handler;
        sigemptyset(&sigact.sa_mask);

        sigaction(SIGFPE, &sigact, 0); //FPU
        sigaction(SIGILL, &sigact, 0); // wrong instruction
        sigaction(SIGSEGV, &sigact, 0); //segfault
        sigaction(SIGBUS, &sigact, 0); // bus memory error
        sigaction(SIGTERM, &sigact, 0);
        sigaction(SIGUSR1, &sigact, 0);

        sigemptyset(&sigset);

        //sigaddset(&sigset, SIGQUIT);
        //sigaddset(&sigset, SIGINT);
        //sigaddset(&sigset, SIGTERM);
        //sigprocmask(SIG_BLOCK, &sigset, NULL);

        struct rlimit lim;
        #define FD_LIMIT 1024*10

        lim.rlim_cur = FD_LIMIT;
        lim.rlim_max = FD_LIMIT;
        setrlimit(RLIMIT_NOFILE, &lim);

        m_sSysLogger->log("Starting work process...");
        //start the threads
        status = (*m_sStartFunc)();
        m_sSysLogger->log("Start work process done");

        if (!status) {
            for (; ;) {
                sigwait(&sigset, &signo);

                if (signo == SIGUSR1) {
                    //reread config
                    if (m_sRereadCfgFun != NULL)
                        (*m_sRereadCfgFun)();
                } else {
                    break;
                }
            }

            //close all
            //	SenderDaemonStopWork() ;
        } else {
            m_sSysLogger->log("Create work thread failed");
        }

        m_sSysLogger->log("[DAEMON] Stopped");
        return CHILD_NEED_TERMINATE;
    }

    void Demonizer::startWithMonitoring(int(*startFunc)(void),
                                        int(*stopFunc)(void), int(*rereadCfgFun)(void)) {

        this->m_sStartFunc = startFunc;
        this->m_sStopFunc = stopFunc;
        this->m_sRereadCfgFun = rereadCfgFun;

        int pid = 0;
        int status = 0;
        int need_start = 1;
        sigset_t sigset;
        siginfo_t siginfo;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGQUIT);
        sigaddset(&sigset, SIGINT);
        //sigaddset(&sigset, SIGTERM);
        //sigaddset(&sigset, SIGCHLD);
        sigaddset(&sigset, SIGCHLD);
        sigprocmask(SIG_BLOCK, &sigset, NULL);

        for (; ;) {
            if (need_start) {
                pid = fork();
                if (pid != 0) {
                    m_sSysLogger->log("Fork with pid=" + Logger::itos(pid));

                    if (PlatformFactory::getInstance()->checkRunningAndSavePID(
                            getName() + "_worker", pid)) {
                        m_sSysLogger->log("worker daemon is already running");
                        exit(CHILD_NEED_TERMINATE);

                    }
                }

            }
            need_start = 1;
            if (pid == -1) {
                m_sSysLogger->log("Monitor: fork failed with " + string(strerror(errno)));
            } else if (!pid) {
                //we are child
                status = this->workProc();
                exit(status);
            } else {// parent
                
                sigwaitinfo(&sigset, &siginfo);
                m_sSysLogger->log("Monitor: wait status...");
                if (siginfo.si_signo == SIGCHLD) {

                    m_sSysLogger->log("Monitor: got child status...");
                    wait(&status);

                    m_sSysLogger->log("Monitor: got exit status");

                    status = WEXITSTATUS(status);
                    if (status == CHILD_NEED_TERMINATE) {
                        m_sSysLogger->log("Monitor: children stopped");
                        break;
                    } else if (status == CHILD_NEED_RESTART) {// restart
                        m_sSysLogger->log("Monitor: children restart");
                    }
                } else if (siginfo.si_signo == SIGUSR1) {//reread config
                    m_sSysLogger->log(
                            "Monitor: resend signal to pid=" + Logger::itos(
                                    pid));
                    kill(pid, SIGUSR1); //resend signal
                    need_start = 0; //don't restart
                } else {
                    m_sSysLogger->log(
                            "Monitor: signal "
                            + string(strsignal(siginfo.si_signo)));
                    //kill child
                    kill(pid, SIGTERM);
                    status = 0;
                    break;
                }
            }
        }
        m_sSysLogger->log("Monitor: stopped");
        //delete pid file
        //unlink(PlatformFactory::getInstance()->calculateFilenameToStorePID(this->getName()));
    }

}