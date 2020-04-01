#ifndef OPENMW_MWWORLD_FOREIGNCELL_H
#define OPENMW_MWWORLD_FOREIGNCELL_H

#include <string>
#include <map>

#include <extern/esm4/reader.hpp>

#include <components/esm/loadcell.hpp>
#include <components/esm/cellid.hpp>

namespace ESM4
{
    struct Cell;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace MWWorld
{
    // We are masquerading as a TES3 cell, i.e.:
    //
    //         ESM::Cell
    //             ^
    //             |
    //          (is a)
    //             |
    //        ForeignCell
    //             o
    //             |
    //             +--(has a)-- ESM4::Cell
    //
    // The old design used to have separate methods (ctor's, getCell vs getForeignCell, etc)
    //
    //         ESM4::Cell      ESM:::Cell
    //             ^
    //             |
    //          (is a)
    //             |
    //        ForeignCell
    //
    struct ForeignCell : public ESM::Cell
    {
        static unsigned int sRecordId;

        ESM4::Cell *mCell; // created in preload() and destroyed in dtor
        bool mHasChildren;

        std::map<std::int32_t, std::uint32_t> mRefrEstimates;
        std::map<std::int32_t, std::uint32_t> mRefrCounts;

        ForeignCell();
        ~ForeignCell();

        std::string mId;          // cache converted string
        //std::string mWorldFormId; // cache converted string (parent worldspace)
        //std::string mName;        // full name

        //std::string mCellId; // for region map (#x y for most exterior cells)
        //std::string mRegion; // for region map, probably will be removed

        // FIXME: should this be a pair with the file index? Maybe not since the context does
        // have the mod index so the file load position can be worked out anyway?
        std::vector<ESM4::ReaderContext> mModList; // contexts of all the files that modify this cell

        bool mIsInterior;
        bool mHasGrid;    // some external cells do not have grid info

        void preload (ESM4::Reader& reader);
        void addFileContext (const ESM4::ReaderContext& ctx);

        // methods to behave like an ESM::Cell
        void load (ESM::ESMReader& esm, bool isDeleted = false);
        void save (ESM::ESMWriter& esm, bool isDeleted = false) const {} // FIXME: TODO

        std::string getDescription() const;
        bool isExterior() const { return !mIsInterior; }
        int getGridX() const;
        int getGridY() const;
        ESM::CellId getCellId() const;

        void setRefrEstimate(std::int32_t groupType, std::uint32_t estimate);
        std::uint32_t getRefrEstimate(std::int32_t groupType) const;
        void incrementRefrCount(std::int32_t groupType);
        std::uint32_t getPersistentRefrCount() const;

        void blank(); // FIXME: is this needed?

        // for testing
        void testPreload (ESM::ESMReader& esm); // FIXME: testing only
    };
}

#endif // OPENMW_MWWORLD_FOREIGNCELL_H
