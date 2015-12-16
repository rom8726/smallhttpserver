#include "demonizer.h"
#include "syslog_logger.h"
#include "tools.h"

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <execinfo.h>
#include <wait.h>

using namespace std;
using namespace Common;

namespace System {

    //----------------------------------------------------------------------
    int (*Demonizer::m_sStartFunc)() = NULL;
    int (*Demonizer::m_sStopFunc)() = NULL;
    int (*Demonizer::m_sRereadCfgFun)() = NULL;
    static SyslogLogger sysLogger;

    //----------------------------------------------------------------------
    Demonizer::Demonizer() {
    }

    //----------------------------------------------------------------------
    Demonizer::~Demonizer() {
    }

    //----------------------------------------------------------------------
    void Demonizer::signal_handler(int sig, siginfo_t *si, void *ptr) {

        void *errorAddr;
        void *trace[16];
        int x;
        int traceSize;
        char **messages;
        (void) si;

        sysLogger.log("Caught signal:" + string(strsignal(sig)));

        if (sig == SIGUSR1) {
            sysLogger.log("Received user signal.");
            if (m_sRereadCfgFun != NULL)
                (*m_sRereadCfgFun)();

            return;
        }


        if (sig == SIGTERM) {
            sysLogger.log("Received sigterm signal. Stopping...");
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
            sysLogger.log("== Backtrace ==");
            for (x = 1; x < traceSize; x++) {
                sysLogger.log(messages[x]);
            }
            sysLogger.log("== End Backtrace ==");
            free(messages);
        }
        sysLogger.log("Stopped");

        if (m_sStopFunc != NULL)
            (*m_sStopFunc)();
        //we need child restart
        exit(CHILD_NEED_RESTART);
    }


    //----------------------------------------------------------------------
    void Demonizer::setup() throw(DemonizerException) {

        sysLogger.setName(this->getName());
        sysLogger.log("Configuring daemon...");
        sysLogger.log(std::string("current PID = ") + Logger::itos(getpid()));

        pid_t pid;
        struct rlimit limits;
        struct sigaction sa;

        umask(0);

//        if (getrlimit(RLIMIT_NOFILE, &limits) < 0) {
//            throw DemonizerException("can`t get RLIMIT_NOFILE");
//        }

        //fork process and disconnect it from parent
        if ((pid = fork()) < 0) {
            throw DemonizerException("fork error");
        } else if (0 != pid) {
            sysLogger.log(std::string("PID1 = ") + Logger::itos(pid));
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
            sysLogger.log(std::string("PID2 = ") + Logger::itos(pid));
            exit(0);
        }

        //chdir to /
        if (chdir("/") < 0) {
            throw DemonizerException("can`t chdir() to /");
        }

        //close all resources
        if (limits.rlim_max == RLIM_INFINITY)
            limits.rlim_max = 1024;

//        u_int32_t idx;
//        for (idx = 0; idx < limits.rlim_max; ++idx) {
//            close(idx);
//        }

        //reopen stdout to /dev/null and another strems to it
        int fd0 = open("/dev/null", O_RDWR);
        int fd1 = dup(0);
        int fd2 = dup(0);

        if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
            /*
            throw DemonizerException("bad file descriptors: " +
                                             Logger::itos(fd0) + " " +
                                             Logger::itos(fd1) + " " +
                                             Logger::itos(fd2));
                                             */
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
        }

        sysLogger.log("Prepare to be daemon ok");

        //check the running -  and save new! pid
        if (SystemTools::checkRunningAndSavePID(this->getName())) {
            sysLogger.log("Daemon is already running");
            throw DemonizerException("Error. Daemon is already running");
        }

        //set handler to signal
        //signal(SIGTERM, signal_handler);
        sysLogger.log("Daemonize done");
    }

    //----------------------------------------------------------------------
    void Demonizer::stop() throw(DemonizerException) {

        //get pid from pid file of running daemon
        int pid = SystemTools::findPID(this->getName());
        //check running
        if (pid == -1) {
            sysLogger.log("Error. Daemon is not running");
            throw DemonizerException("Daemon is not running");
        } else {
            //kill it
            SystemTools::kill(pid);
        }
    }

    //----------------------------------------------------------------------
    void Demonizer::stopWorker() throw(DemonizerException) {

        //get pid from pid file of running daemon
        int pid = SystemTools::findPID(this->getName() + "_worker");
        //check running
        if (pid == -1) {
            sysLogger.log("Error. Worker daemon is not running");
            throw DemonizerException("Worker daemon is not running");
        } else {
            //kill it
            SystemTools::kill(pid);
        }
    }

    //----------------------------------------------------------------------
    void Demonizer::sendUserSignalToWorker() throw(DemonizerException) {

        //get pid from pid file of running daemon
        int pid = SystemTools::findPID(this->getName() + "_worker");
        //check running
        if (pid == -1) {
            sysLogger.log("Error. Worker daemon is not running");
            throw DemonizerException("Worker daemon is not running");
        } else {
            //kill it
            SystemTools::killWithSignal(pid, SIGUSR1);
        }
    }

    //----------------------------------------------------------------------
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

        sysLogger.log("Starting work process...");
        //start the threads
        status = (*m_sStartFunc)();
        sysLogger.log("Start work process done");

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
            sysLogger.log("Create work thread failed");
        }

        sysLogger.log("[DAEMON] Stopped");
        return CHILD_NEED_TERMINATE;
    }

    //----------------------------------------------------------------------
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
                    sysLogger.log("Fork with pid=" + Logger::itos(pid));

                    if (SystemTools::checkRunningAndSavePID(getName() + "_worker", pid)) {
                        sysLogger.log("worker daemon is already running");
                        exit(CHILD_NEED_TERMINATE);
                    }
                }
            }
            need_start = 1;
            if (pid == -1) {
                sysLogger.log("Monitor: fork failed with " + string(strerror(errno)));
            } else if (!pid) {
                //we are child
                status = this->workProc();
                exit(status);
            } else {// parent

                sigwaitinfo(&sigset, &siginfo);
                sysLogger.log("Monitor: wait status...");
                if (siginfo.si_signo == SIGCHLD) {

                    sysLogger.log("Monitor: got child status...");
                    wait(&status);

                    sysLogger.log("Monitor: got exit status");

                    status = WEXITSTATUS(status);
                    if (status == CHILD_NEED_TERMINATE) {
                        sysLogger.log("Monitor: children stopped");
                        break;
                    } else if (status == CHILD_NEED_RESTART) {// restart
                        sysLogger.log("Monitor: children restart");
                    }
                } else if (siginfo.si_signo == SIGUSR1) {//reread config
                    sysLogger.log("Monitor: resend signal to pid=" + Logger::itos(pid));
                    kill(pid, SIGUSR1); //resend signal
                    need_start = 0; //don't restart
                } else {
                    sysLogger.log("Monitor: signal " + string(strsignal(siginfo.si_signo)));
                    //kill child
                    kill(pid, SIGTERM);
                    status = 0;
                    break;
                }
            }
        }
        sysLogger.log("Monitor: stopped");
        //delete pid file
        //unlink(calculateFilenameToStorePID(this->getName()));
    }

}