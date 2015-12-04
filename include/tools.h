#ifndef __COMMON_TOOLS_H__
#define __COMMON_TOOLS_H__

#include "non_copyable.h"

namespace Common {

    //----------------------------------------------------------------------
    class BoolFlagInvertor final
            : private NonCopyable
    {
    public:
        BoolFlagInvertor(volatile bool *pFlag)
                : m_pFlag(pFlag) {
        }

        ~BoolFlagInvertor() {
            *m_pFlag = !*m_pFlag;
        }

    private:
        bool volatile *m_pFlag;
    };

}

#endif  // !__COMMON_TOOLS_H__
