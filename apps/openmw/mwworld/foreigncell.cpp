#include "foreigncell.hpp"

#include <extern/esm4/formid.hpp>
#include <extern/esm4/reader.hpp>
#include <extern/esm4/cell.hpp>

#include <components/esm/esm4reader.hpp>

MWWorld::ForeignCell::ForeignCell() : mCell(0), mHasChildren(false),
                                      mIsInterior(false), mHasGrid(false)
{
}

MWWorld::ForeignCell::~ForeignCell()
{
    if (mCell)
        delete mCell;
}


// FIXME: code for testing
// also see ESMStore::loadTes4Record() and Store<MWWorld::ForeignCell>::testPreload()
void MWWorld::ForeignCell::testPreload(ESM::ESMReader& esm)
{
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

    ESM4::ReaderContext ctx = static_cast<ESM::ESM4Reader*>(&esm)->getESM4Context();
    reader.restoreContext(mModList.back()); // should ensure that mModList is not empty
    // need a method for merging cell sub records
    mCell->load(reader); // FIXME: with the new loading scheme this won't do anything
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
    //assert(!mCell && "ForeignCell: ESM4::Cell already exists");

    if (reader.grp().type == ESM4::Grp_InteriorSubCell)
        mIsInterior = true;

    if (reader.hasCellGrid())
        mHasGrid = true;

    mCell = new ESM4::Cell; // deleted in dtor

    mHasChildren = mCell->preload(reader);

    if ((mCell->mCellFlags & ESM4::CELL_Interior) != 0)
    {
        mData.mFlags |= ESM::Cell::Interior;

        // NOTE: Can't update values from any lighting templates here because during preload
        //       the World is not built yet (i.e. can't get access to ESMStore).
        //       Instead do it in RenderingManager::configureAmbient().
        mAmbi.mAmbient = mCell->mLighting.ambient;
        mAmbi.mSunlight = mCell->mLighting.ambient;
        mAmbi.mFog = mCell->mLighting.fogColor;
        //mAmbi.mFogDensity = (float)(mCell->mLighting.unknown1)/(float)256; // ?
    }
    mAmbi.mFogDensity = 1.f; // FIXME: experiment
}

void MWWorld::ForeignCell::addFileContext(const ESM4::ReaderContext& ctx)
{
    mModList.push_back(ctx);
}

void MWWorld::ForeignCell::setRefrEstimate(std::int32_t groupType, std::uint32_t estimate)
{
    mRefrEstimates[groupType] = estimate;
}

std::uint32_t MWWorld::ForeignCell::getRefrEstimate(std::int32_t groupType) const
{
    std::map<std::int32_t, std::uint32_t>::const_iterator it = mRefrEstimates.find(groupType);

    if (it != mRefrEstimates.end())
        return it->second;

    return 0;
}

void MWWorld::ForeignCell::incrementRefrCount(std::int32_t groupType)
{
    mRefrCounts[groupType] += 1;
}

std::uint32_t MWWorld::ForeignCell::getPersistentRefrCount() const
{
    std::map<std::int32_t, std::uint32_t>::const_iterator it = mRefrCounts.find(ESM4::Grp_CellPersistentChild);

    if (it != mRefrCounts.end())
        return it->second;

    return 0;
}

std::string MWWorld::ForeignCell::getDescription() const
{
    if (mIsInterior)
        return mCell->mEditorId;
    else
    {
        std::ostringstream stream;
        stream << mCell->mX << ", " << mCell->mY;
        return stream.str();
    }
}

int MWWorld::ForeignCell::getGridX() const
{
    return mCell->mX;
}

int MWWorld::ForeignCell::getGridY() const
{
    return mCell->mY;
}

// This is needed for CellStore operator== and operator!= to work
ESM::CellId MWWorld::ForeignCell::getCellId() const
{
    ESM::CellId id;

    id.mIndex.mX = mCell->mX;
    id.mIndex.mY = mCell->mY;

    id.mWorldspace = ESM4::formIdToString(mCell->mFormId);
    id.mPaged = !mIsInterior;

    return id;
}

void MWWorld::ForeignCell::blank()
{
    // FIXME: TODO
}
