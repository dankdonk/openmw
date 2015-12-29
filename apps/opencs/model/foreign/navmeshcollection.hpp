#ifndef CSM_FOREIGN_NAVMESHCOLLECTION_H
#define CSM_FOREIGN_NAVMESHCOLLECTION_H

#include "../world/collection.hpp"

#include "navmesh.hpp"

namespace ESM4
{
    class Reader;
    typedef std::uint32_t FormId;
}

namespace CSMWorld
{
    struct Cell;

    template<typename AT>
    struct IdAccessor;

    template<typename T, typename AT>
    class IdCollection;

    class UniversalId;

    template<>
    void Collection<CSMForeign::NavMesh, IdAccessor<CSMForeign::NavMesh> >::removeRows (int index, int count);

    template<>
    void Collection<CSMForeign::NavMesh, IdAccessor<CSMForeign::NavMesh> >::insertRecord (std::unique_ptr<RecordBase> record,
        int index, UniversalId::Type type);
}

namespace CSMForeign
{
    class NavMeshCollection : public CSMWorld::Collection<NavMesh, CSMWorld::IdAccessor<NavMesh> >
    {
        const CSMWorld::IdCollection<CSMWorld::Cell, CSMWorld::IdAccessor<CSMWorld::Cell> >& mCells;
        //NavMesh mNavMesh;

        std::map<ESM4::FormId, int> mNavMeshIndex;

    public:
        NavMeshCollection (const CSMWorld::IdCollection<CSMWorld::Cell, CSMWorld::IdAccessor<CSMWorld::Cell> >& cells);
        ~NavMeshCollection ();

        // similar to IdCollection but with ESM4::Reader
        int load (ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const NavMesh& record, bool base, int index = -2);

        virtual void loadRecord (NavMesh& record, ESM4::Reader& reader);

        virtual void removeRows (int index, int count);

        virtual int searchId (const std::string& id) const;

        virtual void insertRecord (std::unique_ptr<CSMWorld::RecordBase> record,
                                   int index,
                                   CSMWorld::UniversalId::Type type = CSMWorld::UniversalId::Type_None);
    private:
        NavMeshCollection ();
        NavMeshCollection (const NavMeshCollection& other);
        NavMeshCollection& operator= (const NavMeshCollection& other);

        int getIndex (ESM4::FormId id) const;

        int searchId (ESM4::FormId id) const;
    };
}
#endif // CSM_FOREIGN_NAVMESHCOLLECTION_H
