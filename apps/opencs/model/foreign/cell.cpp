#include "cell.hpp"

#include <sstream>

void CSMForeign::Cell::load (ESM4::Reader& reader)
{
    //mName = mId;

    ESM4::Cell::load (reader);

    //if (!(mData.mFlags & Interior)) // FIXME
    {
        std::ostringstream stream;

        //stream << "#" << mData.mX << " " << mData.mY;

        mId = stream.str();
    }
}

void CSMForeign::Cell::blank()
{
}
