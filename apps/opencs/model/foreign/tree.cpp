#include "tree.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Tree::sRecordId = ESM4::REC_TREE;

CSMForeign::Tree::Tree()
{
}

CSMForeign::Tree::~Tree()
{
}

void CSMForeign::Tree::load(ESM4::Reader& reader)
{
    ESM4::Tree::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Tree::blank()
{
    // FIXME: TODO
}
