#ifndef CSM_FOREIGN_CELLCOLLECTION_H
#define CSM_FOREIGN_CELLCOLLECTION_H

#include <map>
#include <cstdint>

#include "../world/collection.hpp"
#include "../world/record.hpp"

#include "cell.hpp"

namespace ESM4
{
    class Reader;
    typedef std::uint32_t FormId;
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
    class WorldCollection;

    class CellCollection : public CSMWorld::Collection<Cell, CSMWorld::IdAccessor<Cell> >//, public NestedCollection
    {
        WorldCollection& mWorlds; // for looking up World name strings (FULL or EDID) and to register

        // key - x/y coordinates, value - cell formid
        typedef std::map<std::pair<int, int>, ESM4::FormId> CoordinateIndex;

        // key - world formId, value - map of cells
        std::map<ESM4::FormId, CoordinateIndex> mPositionIndex;

        typedef std::map<ESM4::FormId, int> CellIndexMap;
        CellIndexMap mCellIndex;

    public:
        CellCollection (WorldCollection& worlds);
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

        int searchId (ESM4::FormId id) const;

        ESM4::FormId searchFormId (std::int16_t x, std::int16_t y, ESM4::FormId world = /*Tamriel*/0x3c) const;

        Cell *getCell(ESM4::FormId formId); // for updating cell children

    private:
        CellCollection ();
        CellCollection (const CellCollection& other);
        CellCollection& operator= (const CellCollection& other);

        int getIndex (ESM4::FormId id) const;
    };
}
#endif // CSM_FOREIGN_CELLCOLLECTION_H
