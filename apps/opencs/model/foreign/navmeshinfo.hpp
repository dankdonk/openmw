#ifndef CSM_FOREIGN_NAVMESHINFO_H
#define CSM_FOREIGN_NAVMESHINFO_H

#include <string>

#include <extern/esm4/navi.hpp>

namespace CSMForeign
{
    // wrapper class for use with Collection<T>
    struct NavMeshInfo : public ESM4::Navigation::NavMeshInfo
    {
        static unsigned int sRecordId;

        std::string mId;

        void load(const ESM4::Navigation::NavMeshInfo& nvmi);

        void blank();
    };
}

#endif // CSM_FOREIGN_NAVMESHINFO_H
