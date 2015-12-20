#include "navigationcollection.hpp"

#include <iostream> // FIXME
#include <sstream>

#include <extern/esm4/reader.hpp>

#include "../world/idcollection.hpp"
#include "../world/cell.hpp"
#include "../world/record.hpp"

CSMForeign::NavigationCollection::NavigationCollection (const CSMWorld::IdCollection<CSMWorld::Cell>& cells)
  : mCells (cells)
{
}

CSMForeign::NavigationCollection::~NavigationCollection ()
{
}

// Unlike other records there is only one instance of NAVI record in an ESM4 file.  (it might
// be a global summary perhaps?)
//
// Consequently a different loading strategy is required.  A simple, if a little inefficient,
// way is to load the entire record then create the records required by OpenCS.  Retrieval of
// the records can be aided using a map of the NVMI sub-records keyed by cell Id's.
//
//                            1          n
//         ESM4::Navigation o-------------- ESM4::Navigation::NavMeshInfo
//                |                                                 ^
//                |                                                 |
//                |                                1  n             |
//                |       Collection<NavMeshInfo> o----- Record<NavMeshInfo>
//                |          ^                                    o
//                |          |                                    |
//       1  1     o          |                                    |
// Data o----- NavigationCollection                       formid string (mId)
//                   o    o
//                   |    |
//                   |    +--- map<cell id string, vector<formid string>>
//                   |
//                   |
//              Collection<Cell>&
//
//
void CSMForeign::NavigationCollection::load (ESM4::Reader& reader, bool base) // FIXME: remove base?
{
    mNavigation.load(reader); // load the whole record first

    // copy each NVMI subrecord to OpenCS record format
    for (std::vector<ESM4::Navigation::NavMeshInfo>::iterator it = mNavigation.mNavMeshInfo.begin();
            it != mNavigation.mNavMeshInfo.end(); ++it)
    {
        CSMForeign::NavMeshInfo record;

        std::ostringstream ss;
        ss << std::hex << (*it).formId;
        std::string id = ss.str();
        int index = this->searchId(id);

        if (index == -1)
            CSMWorld::IdAccessor<CSMForeign::NavMeshInfo>().getId(record) = id; // new record; set id
        else
        {
            record = this->getRecord(index).get(); // exsiting record
        }

        //loadRecord(record, reader); // FIXME
        record.load(*it);

        (void)load(record, base, index);

        // update cell map
        if ((*it).worldSpaceId == ESM4::FLG_Interior)
        {
            // FIXME: how to map internal cell names to formids?
            std::cout << "ignoring interior worldspace " << std::hex << (*it).worldSpaceId << std::endl;
        }
        else if ((*it).worldSpaceId == ESM4::FLG_Morrowind)
        {
            // external cell; convert to OpenCS cell id format
            std::ostringstream stream;
            stream << "#" << std::floor((float)(*it).cellGrid.grid.x/2)
                   << " " << std::floor((float)(*it).cellGrid.grid.y/2);

            std::map<std::string, std::vector<std::string> >::iterator iter =
                mCellToFormIds.find(stream.str());

            // if new cell, insert a blank entry
            if (iter == mCellToFormIds.end())
            {
                std::pair<std::map<std::string, std::vector<std::string> >::iterator, bool> res =
                    mCellToFormIds.insert(std::make_pair(stream.str(), std::vector<std::string>()));
                if (!res.second)
                    throw std::runtime_error("Navigation: cell to formid map insert failed.");
                iter = res.first;
            }

            // add the formid to the cell (key) for fast lookup later (rendering,  pathfinding)
            (*iter).second.push_back(id);
        }
        else
            std::cout << "ignoring unknown worldspace " << std::hex << (*it).worldSpaceId << std::endl;
    }

    for (std::map<std::string, std::vector<std::string> >::iterator it = mCellToFormIds.begin();
            it != mCellToFormIds.end(); ++it)
    {
        std::cout << it->first;
        for (unsigned int i = 0; i < it->second.size(); ++i)
        {
            std::cout << ", 0x" << it->second[i];
        }
        std::cout << std::endl;
    }
// FIXME: debugging only
#if 0
    const std::vector<Record<CSMForeign::NavMeshInfo> >& records = this->getRecords();
    for (std::vector<Record<CSMForeign::NavMeshInfo> >::const_iterator it = records.begin();
            it != records.end(); ++it)
    {
        std::cout << "test: formid 0x" << std::hex << (*it).get().formId << std::endl;
    }
#endif
}

//void CSMForeign::NavigationCollection::loadRecord (CSMForeign::NavMeshInfo& record,
    //ESM4::Reader& reader)
//{
    //record.load(reader, mCells);
//}

int CSMForeign::NavigationCollection::load (const CSMForeign::NavMeshInfo& record, bool base, int index)
{
    if (index == -2)
        index = this->searchId(CSMWorld::IdAccessor<CSMForeign::NavMeshInfo>().getId(record));

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<CSMForeign::NavMeshInfo> > record2(
                new CSMWorld::Record<CSMForeign::NavMeshInfo>);

        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<CSMForeign::NavMeshInfo> > record2(
                new CSMWorld::Record<CSMForeign::NavMeshInfo>(
                    CSMWorld::Collection<CSMForeign::NavMeshInfo, CSMWorld::IdAccessor<CSMForeign::NavMeshInfo> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

#if 0
CSMForeign::NavigationCollection::NavigationCollection ()
{
}

CSMForeign::NavigationCollection::~NavigationCollection ()
{
}

void CSMForeign::NavigationCollection::load (ESM4::Reader& reader, bool base)
{
    mNavigation.load(reader);
}
#endif
