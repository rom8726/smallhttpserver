#ifndef __COMMON_NON_COPYABLE_H__
#define __COMMON_NON_COPYABLE_H__

namespace Common {

    //----------------------------------------------------------------------
    class NonCopyable {
    public:
        NonCopyable(const NonCopyable &) = delete;

        NonCopyable &operator=(const NonCopyable &) = delete;

        NonCopyable() { }

    protected:
        ~NonCopyable() { }
    };

}

#endif  // !__COMMON_NON_COPYABLE_H__
