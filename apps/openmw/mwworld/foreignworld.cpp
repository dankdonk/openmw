#include "foreignworld.hpp"

//#include <iostream> // FIXME: testing only

#include <OgreResourceGroupManager.h>

#include <components/esm/esm4reader.hpp>

#include <extern/esm4/reader.hpp>
#include <extern/esm4/cell.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "esmstore.hpp"
#include "cellstore.hpp"

MWWorld::ForeignWorld::ForeignWorld() : mDummyCell(nullptr)
{
}

MWWorld::ForeignWorld::~ForeignWorld()
{
}

void MWWorld::ForeignWorld::load(ESM::ESMReader& esm, bool isDeleted)
{
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

    ESM4::World::load(reader);

    //ESM4::formIdToString(mFormId, mId);
    //ESM4::formIdToString(mParent, mWorldFormId);

    //mName = mFullName;
}

bool MWWorld::ForeignWorld::updateCellGridMap(std::int16_t x, std::int16_t y, ESM4::FormId formId)
{
    std::pair<std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>::iterator, bool> ret
        = mCellGridMap.insert(std::make_pair(std::make_pair(x, y), formId));

    // should be the same formid, check just in case
    if (!ret.second)
    {
        ESM4::FormId oldId = ret.first->second;

        if (oldId != formId)
            throw std::runtime_error("Cell GridMap different formid found");
    }

    return ret.second;
}

const std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>& MWWorld::ForeignWorld::getCellGridMap() const
{
    return mCellGridMap;
}

// not used
ESM4::FormId MWWorld::ForeignWorld::getCellId(std::int16_t x, std::int16_t y) const
{
    std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>::const_iterator it
        = mCellGridMap.find(std::make_pair(x, y));

    if (it != mCellGridMap.end())
        return it->second;

    return 0;
}

bool MWWorld::ForeignWorld::setDummyCell(ForeignCell *cell)
{
    if (mDummyCell)
    {
        if (cell->mCell->mFormId != mDummyCell->mCell->mFormId)
            throw std::runtime_error("Dummy cell different formid found");

        return false;
    }

    mDummyCell = cell;

    return true;
}

MWWorld::ForeignCell *MWWorld::ForeignWorld::getDummyCell() const
{
    return mDummyCell;
}

// added from Cells::initNewWorld()
void MWWorld::ForeignWorld::addVisibleDistGrids(const std::string& cmpFile, const std::string& group)
{
    std::size_t pos = cmpFile.find(".cmp");
    if (pos == std::string::npos)
        return; // FIXME: throw?

    std::string worldEditorId = cmpFile.substr(11, pos-11); // 11 for 'distantlod\'

    //std::cout << worldEditorId << " " << group << " " << cmpFile << std::endl; // FIXME

    const Ogre::ResourceGroupManager& groupMgr = Ogre::ResourceGroupManager::getSingleton();
    Ogre::DataStreamPtr file = groupMgr.openResource(cmpFile, group);
    if (!file)
        return;

    // NOTE: if called multiple times only the last cmpFile isused
    mVisibleDistGrids.clear();
    mVisibleDistRefs.clear();

    while (!file->eof())
    {
        std::int16_t x, y;
        file->read(&y, sizeof(y)); // NOTE: that y comes before x
        file->read(&x, sizeof(x));
        mVisibleDistGrids.push_back(std::pair<std::int16_t, std::int16_t>(x, y));

        //std::cout << "(" << x << "," << y << ")" << std::endl; // FIXME
    }
    // FIXME: either don't bother with the last entry or do something with it

    for (std::size_t i = 0; i < mVisibleDistGrids.size()-1; ++i) // ignore the last one
    {
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::int16_t x = mVisibleDistGrids[i].first;
        std::int16_t y = mVisibleDistGrids[i].second;

        std::string lodFile
            = "distantlod\\" + worldEditorId + "_" + std::to_string(x) + "_" + std::to_string(y) + ".lod";

        std::pair<std::int16_t, std::int16_t> grid(x, y);
        std::map<std::pair<std::int16_t, std::int16_t>, std::vector<ESM4::LODReference> >::iterator lb
            = mVisibleDistRefs.lower_bound(grid);

        if (lb == mVisibleDistRefs.end() || (mVisibleDistRefs.key_comp()(grid, lb->first)))
            lb = mVisibleDistRefs.insert(lb, { grid, std::vector<ESM4::LODReference>() }); // insert a new world

        try
        {
            Ogre::DataStreamPtr file = groupMgr.openResource(lodFile, group);

            std::uint32_t numObj;
            file->read(&numObj, sizeof(numObj));

            for (std::size_t j = 0; j < numObj; ++j)
            {
                ESM4::FormId baseId;
                file->read(&(baseId), sizeof(ESM4::FormId));

                bool isStat = false;
                if (store.getRecordType(baseId) == ESM4::REC_STAT) // FIXME: support other types
                    isStat = true;

                std::uint32_t numInst;
                file->read(&numInst, sizeof(numInst));

                std::vector<ESM4::LODReference> instances;
                instances.resize(numInst);

                for (std::size_t k = 0; k < numInst; ++k)
                {
                    ESM4::LODReference r;
                    r.baseObj = baseId;
                    instances[k] = r;
                }

                for (std::size_t k = 0; k < numInst; ++k)
                {
                    file->read(&(instances[k].placement.pos.x), sizeof(float));
                    file->read(&(instances[k].placement.pos.y), sizeof(float));
                    file->read(&(instances[k].placement.pos.z), sizeof(float));
                }

                for (std::size_t k = 0; k < numInst; ++k)
                {
                    file->read(&(instances[k].placement.rot.x), sizeof(float));
                    file->read(&(instances[k].placement.rot.y), sizeof(float));
                    file->read(&(instances[k].placement.rot.z), sizeof(float));
                }

                for (std::size_t k = 0; k < numInst; ++k)
                {
                    file->read(&(instances[k].scale), sizeof(float));

                    // FIXME: these need to be grouped into the grids
                    if (isStat) // FIXME: do other types
                        lb->second.push_back(instances[k]);
                }
            }

            //std::cout << lodFile << " (" << x << "," << y << ")  num obj "
                      //<< mVisibleDistRefs[grid].size() << std::endl; // FIXME
        }
        catch (...)
        {
            // log something?
        }
    }
}

const std::vector<ESM4::LODReference> *MWWorld::ForeignWorld::getVisibleDistRefs(std::int16_t x, std::int16_t y) const
{
    std::map<std::pair<std::int16_t, std::int16_t>, std::vector<ESM4::LODReference> >::const_iterator it
        = mVisibleDistRefs.find(std::make_pair(x, y));

    if (it != mVisibleDistRefs.end())
        return &it->second;

    return nullptr;
}

void MWWorld::ForeignWorld::blank()
{
    // FIXME: TODO
}
