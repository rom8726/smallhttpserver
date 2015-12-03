#ifndef __COMMON_EXCEPTIONS_H__
#define __COMMON_EXCEPTIONS_H__

#include <stdexcept>

#define DECLARE_EXCEPTION_EX(class_, ext_, base_) \
  class class_##ext_ \
    : public base_ \
  { \
  public: \
    template <typename ... Args> \
    class_##ext_(Args const & ... args) \
      : base_(args ... ) \
    { \
    } \
    template <typename ... Args> \
    class_##ext_(int _code, Args const & ... args) \
      : base_(args ... ) \
      , code(_code) \
    { \
    } \
    virtual int getCode() const \
    { \
      return code; \
    } \
  private: \
    int code = 0; \
  };

#define DECLARE_RUNTIME_EXCEPTION(class_) \
  DECLARE_EXCEPTION_EX(class_, Exception, std::runtime_error)

#endif  // !__COMMON_EXCEPTIONS_H__
