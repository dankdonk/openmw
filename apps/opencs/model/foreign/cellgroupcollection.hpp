#ifndef CSM_FOREIGN_CELLGROUPCOLLECTION_H
#define CSM_FOREIGN_CELLGROUPCOLLECTION_H

//#include <map>
#include <cstdint>

#include <extern/esm4/grup.hpp>

#include "collection.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct CellGroup : public ESM4::CellGroup
    {
        ESM4::FormId  mFormId;
        //std::uint32_t mFlags; // for Rec_Deleted flag check in IdCollection
        std::string   mId;

        //CellGroup() : mFormId(0), mFlags(0) { mId.clear(); }
        void blank() {}
    };
    //class WorldCollection;

    class CellGroupCollection : public Collection<CellGroup>
    {
        //WorldCollection& mWorlds; // for looking up World name strings (FULL or EDID) and to register

        // key - x/y coordinates, value - cell formid
        //typedef std::map<std::pair<int, int>, ESM4::FormId> CoordinateIndex;

        // key - world formId, value - map of cells
        //std::map<ESM4::FormId, CoordinateIndex> mPositionIndex;

    public:
        CellGroupCollection ();//WorldCollection& worlds);
        ~CellGroupCollection ();

        // This method is called from CSMWorld::Data::loadTes4Record() for loading (i.e. not skipping)
        // REC_CELL, REC_PGRD, REC_REFR, REC_ACHR and REC_ACRE
        //virtual int load (ESM4::Reader& reader, bool base);

        // This method is called from CSMWorld::Data::loadTes4Record() for skipping REC_CELL,
        // Grp_CellPersistentChild, Grp_CellTemporaryChild and Grp_CellVisibleDistChild
        // ESM4::Reader's shared pointer is saved along with the file offset (and others?)
        void saveContext (ESM4::Reader& reader);

        //ESM4::FormId searchCoord (std::int16_t x, std::int16_t y, ESM4::FormId world = /*Tamriel*/0x3c) const;

        //Cell *getCell(ESM4::FormId formId); // for updating cell children

    private:
        //CellGroupCollection ();
        CellGroupCollection (const CellGroupCollection& other);
        CellGroupCollection& operator= (const CellGroupCollection& other);
    };
}
#endif // CSM_FOREIGN_CELLGROUPCOLLECTION_H
