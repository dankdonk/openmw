#ifndef CSM_FOREIGN_CELLCOLLECTION_H
#define CSM_FOREIGN_CELLCOLLECTION_H

#include <map>
#include <cstdint>

#include "idcollection.hpp"
#include "cell.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class WorldCollection;

    class CellCollection : public IdCollection<Cell>//, public NestedCollection
    {
        WorldCollection& mWorlds; // for looking up World name strings (FULL or EDID) and to register

        // key - x/y coordinates, value - cell formid
        typedef std::map<std::pair<int, int>, ESM4::FormId> CoordinateIndex;

        // key - world formId, value - map of cells
        std::map<ESM4::FormId, CoordinateIndex> mPositionIndex;

    public:
        CellCollection (WorldCollection& worlds);
        ~CellCollection ();

        virtual int load (ESM4::Reader& reader, bool base);

        ESM4::FormId searchCoord (std::int16_t x, std::int16_t y, ESM4::FormId world = /*Tamriel*/0x3c) const;

        Cell *getCell(ESM4::FormId formId); // for updating cell children

    private:
        CellCollection ();
        CellCollection (const CellCollection& other);
        CellCollection& operator= (const CellCollection& other);
    };
}
#endif // CSM_FOREIGN_CELLCOLLECTION_H
