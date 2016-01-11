#include "sound.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Sound::sRecordId = ESM4::REC_SOUN;

CSMForeign::Sound::Sound()
{
}

CSMForeign::Sound::~Sound()
{
}

void CSMForeign::Sound::load(ESM4::Reader& reader)
{
    ESM4::Sound::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Sound::blank()
{
    // FIXME: TODO
}
