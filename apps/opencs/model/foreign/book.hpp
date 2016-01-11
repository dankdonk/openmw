#ifndef CSM_FOREIGN_BOOK_H
#define CSM_FOREIGN_BOOK_H

#include <string>

#include <extern/esm4/book.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Book : public ESM4::Book
    {
        static unsigned int sRecordId;

        Book();
        ~Book();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_BOOK_H
