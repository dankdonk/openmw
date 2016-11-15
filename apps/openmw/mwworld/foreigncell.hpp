#ifndef OPENMW_MWWORLD_FOREIGNCELL_H
#define OPENMW_MWWORLD_FOREIGNCELL_H

#include <string>

#include <extern/esm4/cell.hpp>
#include <extern/esm4/reader.hpp>

namespace ESM
{
    class ESMReader;
    class ESMWriter;
    struct Cell;
}

namespace MWWorld
{
    // Current strategy (see esmstore.cpp and store.cpp):
    //
    // Initially only the cell headers are loaded (for the formId) and stored in ForeignWorld
    // and the context saved in mModList.
    //
    // Later, when a cell is actually required, the cell and its contents (i.e. references) are
    // loaded.
    struct ForeignCell : public ESM4::Cell
    {
        static unsigned int sRecordId;

        bool mHasChildren;
        ESM4::ReaderContext mCellChildContext;

        ForeignCell();
        ~ForeignCell();

        //std::string mId;          // cache converted string
        //std::string mWorldFormId; // cache converted string (parent worldspace)
        //std::string mName;        // full name

        //std::string mCellId; // for region map (#x y for most exterior cells)
        //std::string mRegion; // for region map, probably will be removed

        // FIXME: should this be a pair with the file index? Maybe not since the context does
        // have the mod index so the file load position can be worked out anyway?
        std::vector<ESM4::ReaderContext> mModList; // contexts of all the files that modify this cell

        bool mIsInterior; // FIXME: needs a better way
        bool mHasGrid;    // some external cells do not have grid info

        void preload (ESM4::Reader& reader);
        void addFileContext (const ESM4::ReaderContext& ctx);

        void load (ESM::ESMReader& esm, bool isDeleted = false);
        void save (ESM::ESMWriter& esm, bool isDeleted = false) const {} // FIXME: TODO

        void blank(); // FIXME: is this needed?

        void testPreload (ESM::ESMReader& esm); // FIXME: testing only
    };
}

#endif // OPENMW_MWWORLD_FOREIGNCELL_H
