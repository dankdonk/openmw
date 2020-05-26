#ifndef OPENMW_MWWORLD_FOREIGNWORLD_H
#define OPENMW_MWWORLD_FOREIGNWORLD_H

#include <string>
#include <map>

#include <extern/esm4/wrld.hpp>
#include <extern/esm4/reference.hpp> // LODReference

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
        ESM4::FormId mDummyCellId;
        ForeignCell *mVisibleDistCell;
        CellStore *mVisibleDistCellStore;
        ForeignCell *mDummyCell;
        CellStore *mDummyCellStore; // we don't own this one

        // from ".cmp" files
        std::vector<std::pair<std::int16_t, std::int16_t> > mVisibleDistGrids;
        // from ".lod" files based on the grids in mVisibleDistGrids
        std::map<std::pair<std::int16_t, std::int16_t>, std::vector<ESM4::LODReference> > mVisibleDistRefs;

        // Rather than using ESM4::WorldGroup, keep some variables here
        // (assumed only one per world)
        ESM4::FormId mRoad;

        //std::string mId;          // cache converted string
        //std::string mWorldFormId; // cache converted string (parent worldspace)
        //std::string mName;        // full name

    public:
        ForeignWorld();
        ~ForeignWorld();

        // returns false if insert fails (i.e. grid already exists)
        // throws if the formid is different for the same grid (when insert fails)
        bool updateCellGridMap(std::int16_t x, std::int16_t y, ESM4::FormId formId);

        // returns false if insert fails (i.e. formid already exists)
        // throws if the formid is different (when insert fails)
        bool setDummyCellId(ESM4::FormId formId); // FIXME deprecated
        bool setDummyCell(ForeignCell *cell);

        const std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>& getCellGridMap() const;
        ESM4::FormId getCellId(std::int16_t x, std::int16_t y) const;

        ForeignCell *getDummyCell();
        ForeignCell *getDummyCell() const;

        void addVisibleDistGrids(const std::string& cmpFile, const std::string& group);
        // FIXME: maybe better to get a list of references instead? or even a "load" method?
        inline const std::vector<std::pair<std::int16_t, std::int16_t> >&
                getVisibleDistGrids() const { return mVisibleDistGrids; }
        const std::vector<ESM4::LODReference> *getVisibleDistRefs(std::int16_t x, std::int16_t y) const;

        void load (ESM::ESMReader& esm, bool isDeleted = false);
        void save (ESM::ESMWriter& esm, bool isDeleted = false) const {} // FIXME: TODO

        void blank();
    };
}

#endif // OPENMW_MWWORLD_FOREIGNWORLD_H
