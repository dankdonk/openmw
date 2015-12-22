#ifndef CSM_FOREIGN_LANDSCAPECOLLECTION_H
#define CSM_FOREIGN_LANDSCAPECOLLECTION_H

#include "../world/collection.hpp"
//#include "../world/nestedcollection.hpp"
//#include "../world/record.hpp"
#include "landscape.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class CellCollection;

    class LandscapeCollection : public CSMWorld::Collection<Landscape, CSMWorld::IdAccessor<Landscape> >//, public NestedCollection
    {
        const CSMForeign::CellCollection& mCells;
        //Landscape mNavMesh;

    public:
        LandscapeCollection (const CellCollection& cells);
        ~LandscapeCollection ();

        // similar to IdCollection but with ESM4::Reader
        int load(ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const Landscape& record, bool base, int index = -2);

        virtual void loadRecord (Landscape& record, ESM4::Reader& reader);
#if 0
        virtual void addNestedRow(int row, int col, int position) = 0;

        virtual void removeNestedRows(int row, int column, int subRow) = 0;

        virtual QVariant getNestedData(int row, int column, int subRow, int subColumn) const = 0;

        virtual void setNestedData(int row, int column, const QVariant& data, int subRow, int subColumn) = 0;

        virtual NestedTableWrapperBase* nestedTable(int row, int column) const = 0;

        virtual void setNestedTable(int row, int column, const NestedTableWrapperBase& nestedTable) = 0;

        virtual int getNestedRowsCount(int row, int column) const;

        virtual int getNestedColumnsCount(int row, int column) const;

        virtual NestableColumn *getNestableColumn(int column) = 0;
#endif
    private:
        LandscapeCollection ();
        LandscapeCollection (const LandscapeCollection& other);
        LandscapeCollection& operator= (const LandscapeCollection& other);
    };
}
#endif // CSM_FOREIGN_LANDSCAPECOLLECTION_H
