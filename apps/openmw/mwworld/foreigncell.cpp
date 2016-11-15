#include "foreigncell.hpp"

#include <components/esm/esm4reader.hpp>

#include <extern/esm4/formid.hpp>
#include <extern/esm4/reader.hpp>

unsigned int MWWorld::ForeignCell::sRecordId = ESM4::REC_CELL;

MWWorld::ForeignCell::ForeignCell() : mHasChildren(false), mIsInterior(false), mHasGrid(false)
{
}

MWWorld::ForeignCell::~ForeignCell()
{
}

// FIXME: code for testing
// also see ESMStore::loadTes4Record() and Store<MWWorld::ForeignCell>::testPreload()
void MWWorld::ForeignCell::testPreload(ESM::ESMReader& esm)
{
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

    ESM4::ReaderContext ctx = static_cast<ESM::ESM4Reader*>(&esm)->getESM4Context();
    reader.restoreContext(mModList.back()); // should ensure that mModList is not empty
    ESM4::Cell::load(reader);
    static_cast<ESM::ESM4Reader*>(&esm)->restoreESM4Context(ctx);
}

void MWWorld::ForeignCell::load(ESM::ESMReader& esm, bool isDeleted)
{
    //std::vector<ESM4::ReaderContext> mModList;
#if 0
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

    ESM4::Cell::load(reader);

    if (reader.grp().type == ESM4::Grp_InteriorSubCell)
        mIsInterior = true;

    if (reader.hasCellGrid())
        mHasGrid = true;

    //ESM4::formIdToString(mFormId, mId);
    //ESM4::formIdToString(mParent, mWorldFormId);

    //mName = mFullName;
#endif
}

void MWWorld::ForeignCell::preload (ESM4::Reader& reader)
{
    mHasChildren = ESM4::Cell::preload(reader, mCellChildContext);
}

void MWWorld::ForeignCell::addFileContext(const ESM4::ReaderContext& ctx)
{
    mModList.push_back(ctx);
}

void MWWorld::ForeignCell::blank()
{
    // FIXME: TODO
}
