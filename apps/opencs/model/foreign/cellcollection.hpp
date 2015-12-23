#ifndef CSM_FOREIGN_CELLCOLLECTION_H
#define CSM_FOREIGN_CELLCOLLECTION_H

#include <map>
#include <cstdint>

#include "../world/collection.hpp"
//#include "../world/nestedcollection.hpp"
#include "../world/record.hpp"

#include "cell.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMWorld
{
    class UniversalId;

    template<>
    void Collection<CSMForeign::Cell, IdAccessor<CSMForeign::Cell> >::removeRows (int index, int count);

    template<>
    void Collection<CSMForeign::Cell, IdAccessor<CSMForeign::Cell> >::insertRecord (std::unique_ptr<RecordBase> record,
        int index, UniversalId::Type type);
}

namespace CSMForeign
{
    class CellCollection : public CSMWorld::Collection<Cell, CSMWorld::IdAccessor<Cell> >//, public NestedCollection
    {
        std::map<std::uint32_t, int> mCellIndex;
        std::map<std::string, std::uint32_t> mIdMap;

    public:
        CellCollection ();
        ~CellCollection ();

        // similar to IdCollection but with ESM4::Reader
        int load (ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const Cell& record, bool base, int index = -2);

        virtual void loadRecord (Cell& record, ESM4::Reader& reader);

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
        CellCollection (const CellCollection& other);
        CellCollection& operator= (const CellCollection& other);

        int getIndex (std::uint32_t id) const;

        int searchId (std::uint32_t id) const;
    };
}
#endif // CSM_FOREIGN_CELLCOLLECTION_H
