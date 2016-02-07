#include "esm4reader.hpp"

#include "../files/constrainedfiledatastream.hpp"

ESM::ESM4Reader::ESM4Reader(bool oldHeader)
{
    mReader.setRecHeaderSize(oldHeader ? sizeof(ESM4::RecordHeader)-4 : sizeof(ESM4::RecordHeader));
}

ESM::ESM4Reader::~ESM4Reader()
{
}

void ESM::ESM4Reader::openTes4File(const std::string &name)
{
    // TODO: try using a different implementation, also note that this one throws exceptions
    mCtx.leftFile = mReader.openTes4File(openConstrainedFileDataStream (name.c_str ()), name);
    mReader.registerForUpdates(this); // for updating mCtx.leftFile

    mReader.getRecordHeader();
    if (mReader.hdr().record.typeId == ESM4::REC_TES4)
    {
        mReader.loadHeader();
        mCtx.leftFile -= mReader.hdr().record.dataSize;

        // Hack: copy over values to TES3 header for getVer() and getRecordCount() to work
        mHeader.mData.version = mReader.esmVersion();
        mHeader.mData.records = mReader.numRecords();
    }
    else
        fail("Unknown file format");
}

// callback from mReader
void ESM::ESM4Reader::update(std::size_t size)
{
    mCtx.leftFile -= size;
}
