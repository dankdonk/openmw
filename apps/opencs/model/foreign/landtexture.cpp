#include "landtexture.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::LandTexture::sRecordId = ESM4::REC_LTEX;

CSMForeign::LandTexture::LandTexture()
{
}

CSMForeign::LandTexture::~LandTexture()
{
}

void CSMForeign::LandTexture::load(ESM4::Reader& reader)
{
    ESM4::LandTexture::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::LandTexture::blank()
{
    // FIXME: TODO
}
