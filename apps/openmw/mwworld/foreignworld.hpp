#ifndef OPENMW_MWWORLD_FOREIGNWORLD_H
#define OPENMW_MWWORLD_FOREIGNWORLD_H

#include <string>
#include <map>

#include <extern/esm4/wrld.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace MWWorld
{
    struct ForeignCell;
    class CellStore;

    class ForeignWorld : public ESM4::World
    {
    private:
        // Keep an index of the cells keyed by their grid position (but not for interior cells),
        // so that Store<ForeignWorld> can have a search method to find a cell's formid.
        std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId> mCellGridMap;
        //ESM4::FormId mDummyCell;
        ForeignCell *mVisibleDistCell;
        CellStore *mVisibleDistCellStore;
        ForeignCell *mDummyCell;
        CellStore *mDummyCellStore;

        // Rather than using ESM4::WorldGroup, keep some variables here
        // (assumed only one per world)
        ESM4::FormId mRoad;

        //std::string mId;          // cache converted string
        //std::string mWorldFormId; // cache converted string (parent worldspace)
        //std::string mName;        // full name

    public:
        ForeignWorld();
        ~ForeignWorld();

        // returns false if insert fails (e.g. index already exists)
        bool updateCellGridMap(std::int16_t x, std::int16_t y, ESM4::FormId id);
        //bool setDummyCell(ESM4::FormId id);

        const std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>& getCellGridMap() const;

        // FIXME: this is broken since there can be more than one
        CellStore *getVisibleDistCell();
        CellStore *getVisibleDistCell() const;

        CellStore *getDummyCell();
        CellStore *getDummyCell() const;
        //ESM4::FormId getDummyCell() const { return mDummyCell; }

        void load (ESM::ESMReader& esm, bool isDeleted = false);
        void save (ESM::ESMWriter& esm, bool isDeleted = false) const {} // FIXME: TODO

        void blank();
    };
}

#endif // OPENMW_MWWORLD_FOREIGNWORLD_H
