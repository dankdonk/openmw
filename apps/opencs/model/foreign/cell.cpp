#include "cell.hpp"

#include <sstream>

void CSMForeign::Cell::load (ESM4::Reader& reader)
{
    ESM4::Cell::load (reader);

    mName = mFullName;

#if 0
    if (!(mData.mFlags & Interior)) // FIXME
    {
        std::ostringstream stream;

        //stream << "#" << mData.mX << " " << mData.mY;

        mId = stream.str();
    }
#endif
}

std::string CSMForeign::Cell::getRegion() const
{
    return ""; // FIXME
}

void CSMForeign::Cell::blank()
{
}
