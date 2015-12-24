#ifndef CSM_FOREIGN_NAVMESH_H
#define CSM_FOREIGN_NAVMESH_H

#include <string>

#include <extern/esm4/navm.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMWorld
{
    struct Cell; // FIXME: consider using foreign cell instead

    template<typename T>
    struct IdAccessor;

    template<typename T, typename AT>
    class IdCollection;
}

namespace CSMForeign
{
    struct NavMesh : public ESM4::NavMesh
    {
        static unsigned int sRecordId;

        std::string mId; // required by Collection<T>
        std::string mCell; // Cell name

        void load(ESM4::Reader& esm,
                const CSMWorld::IdCollection<CSMWorld::Cell, CSMWorld::IdAccessor<CSMWorld::Cell> >& cells);

        void load(ESM4::Reader& esm);

        void blank(); // required by Collection<T>
    };
}

#endif // CSM_FOREIGN_NAVMESH_H
