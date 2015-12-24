#include "landscapetexture.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::LandscapeTexture::sRecordId = ESM4::REC_LTEX;

CSMForeign::LandscapeTexture::LandscapeTexture()
{
}

CSMForeign::LandscapeTexture::~LandscapeTexture()
{
}

void CSMForeign::LandscapeTexture::load(ESM4::Reader& reader)
{
    ESM4::LandTexture::load(reader);
}

void CSMForeign::LandscapeTexture::blank()
{
    // FIXME: TODO
}
