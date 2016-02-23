# README #

Small HTTP-server for static content, based on libevent. Uses C++11, Boost library and memcached.
Implemented logger (console and syslog), demonizer (you can start http server as daemon), config in JSON format, multithreading and caching (based on memcached).
The base of this project was an article https://habrahabr.ru/post/217437/

### What is this repository for? ###

* Quick summary
* Version
* [Learn Markdown](https://bitbucket.org/tutorials/markdowndemo)

### How do I get set up? ###

You need to install next packages (Ubuntu):

Install CMake:
http://askubuntu.com/questions/610291/how-to-install-cmake-3-2-on-ubuntu-14-04

Install C++ Compiler:
**sudo apt-get install build-essential**

Install Boost library:
**sudo apt-get install libboost-serialization-dev**

Install Memcached:
**sudo apt-get install libmemcached-dev**

Install libevent:
**sudo apt-get install libevent-dev**

### Contribution guidelines ###

* Writing tests
* Code review
* Other guidelines

### Who do I talk to? ###

* Repo owner or admin
* Other community or team contact
