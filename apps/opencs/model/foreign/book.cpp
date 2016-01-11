#include "book.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Book::sRecordId = ESM4::REC_BOOK;

CSMForeign::Book::Book()
{
}

CSMForeign::Book::~Book()
{
}

void CSMForeign::Book::load(ESM4::Reader& reader)
{
    ESM4::Book::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Book::blank()
{
    // FIXME: TODO
}
