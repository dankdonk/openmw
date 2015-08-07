#include "navmesh.hpp"

#include <sstream>

#include <extern/esm4/reader.hpp>

#include "../world/cell.hpp"
#include "../world/idcollection.hpp"

unsigned int CSMForeign::NavMesh::sRecordId = ESM4::REC_NAVM;

void CSMForeign::NavMesh::load(ESM4::Reader& reader, const CSMWorld::IdCollection<CSMWorld::Cell>& cells)
{
    //ESM::ESM4Reader& reader = static_cast<ESM::ESM4Reader&>(esm)->reader();
    load(reader);

    // correct ID // FIXME
    if (!mId.empty() && mId[0]!='#' && cells.searchId (mId)==-1)
    {
        std::ostringstream stream;

        //stream << "#" << mData.mX << " " << mData.mY;

        mId = stream.str();
    }
}

void CSMForeign::NavMesh::load(ESM4::Reader& reader)
{
    //ESM4::Reader& reader = static_cast<ESM::ESM4Reader&>(esm).reader();
    ESM4::NavMesh::load(reader);

    if (mCell.empty())
    {
        // HACK // FIXME
        std::ostringstream stream;
        stream << "#" << std::floor((float)reader.currCell().grid.x/2)
               << " " << std::floor((float)reader.currCell().grid.y/2);

        mId = stream.str();
    }
    else
        mId = mCell;
}

void CSMForeign::NavMesh::blank()
{
}
