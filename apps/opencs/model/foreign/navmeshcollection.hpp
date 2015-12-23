#ifndef CSM_FOREIGN_NAVMESHCOLLECTION_H
#define CSM_FOREIGN_NAVMESHCOLLECTION_H

#include "../world/collection.hpp"
//#include "../world/nestedcollection.hpp"
//#include "../world/record.hpp"
#include "navmesh.hpp"

namespace ESM4
{
    class Reader;
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
    class NavMeshCollection : public CSMWorld::Collection<NavMesh, CSMWorld::IdAccessor<NavMesh> >//, public NestedCollection
    {
        const CSMWorld::IdCollection<CSMWorld::Cell, CSMWorld::IdAccessor<CSMWorld::Cell> >& mCells;
        //NavMesh mNavMesh;

        std::map<std::uint32_t, int> mNavMeshIndex;

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
#if 0
        virtual void addNestedRow (int row, int col, int position) = 0;

        virtual void removeNestedRows (int row, int column, int subRow) = 0;

        virtual QVariant getNestedData (int row, int column, int subRow, int subColumn) const = 0;

        virtual void setNestedData (int row, int column, const QVariant& data, int subRow, int subColumn) = 0;

        virtual NestedTableWrapperBase* nestedTable (int row, int column) const = 0;

        virtual void setNestedTable (int row, int column, const NestedTableWrapperBase& nestedTable) = 0;

        virtual int getNestedRowsCount (int row, int column) const;

        virtual int getNestedColumnsCount (int row, int column) const;

        virtual NestableColumn *getNestableColumn (int column) = 0;
#endif
    private:
        NavMeshCollection ();
        NavMeshCollection (const NavMeshCollection& other);
        NavMeshCollection& operator= (const NavMeshCollection& other);

        int getIndex (std::uint32_t id) const;

        int searchId (std::uint32_t id) const;
    };
}
#endif // CSM_FOREIGN_NAVMESHCOLLECTION_H
