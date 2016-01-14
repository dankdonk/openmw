#include "ingredient.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Ingredient::sRecordId = ESM4::REC_INGR;

CSMForeign::Ingredient::Ingredient()
{
}

CSMForeign::Ingredient::~Ingredient()
{
}

void CSMForeign::Ingredient::load(ESM4::Reader& reader)
{
    ESM4::Ingredient::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Ingredient::blank()
{
    // FIXME: TODO
}
