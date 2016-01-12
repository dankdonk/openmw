#ifndef COMPONENT_ESM_4READER_H
#define COMPONENT_ESM_4READER_H

#include <extern/esm4/tes4.hpp>
#include <extern/esm4/reader.hpp>

#include "esmreader.hpp"

namespace ESM
{
    // Wrapper class for integrating into OpenCS
    class ESM4Reader : public ESMReader, public ESM4::ReaderObserver
    {
        ESM4::Header mESM4Header;
        ESM4::Reader mReader;

    public:

        ESM4Reader(bool oldHeader = true);
        virtual ~ESM4Reader();

        ESM4::Reader& reader() { return mReader; }

        void openTes4File(const std::string &name);

        virtual void update(std::size_t size);
    };
}
#endif // COMPONENT_ESM_4READER_H
