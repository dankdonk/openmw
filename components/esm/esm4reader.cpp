#include "esm4reader.hpp"

#include "../files/constrainedfiledatastream.hpp"

ESM::ESM4Reader::ESM4Reader(bool oldHeader)
{
    // TES4 header size is 4 bytes smaller than TES5 header
    mReader.setRecHeaderSize(oldHeader ? sizeof(ESM4::RecordHeader)-4 : sizeof(ESM4::RecordHeader));
}

ESM::ESM4Reader::~ESM4Reader()
{
}

void ESM::ESM4Reader::openTes4File(const std::string &name)
{
    mCtx.filename = name;
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

ESM4::ReaderContext ESM::ESM4Reader::getESM4Context()
{
    //std::cout << "context leftFile: " << mCtx.leftFile << std::endl; // FIXME: debug
    return mReader.getContext();
}

void ESM::ESM4Reader::restoreESM4Context(const ESM4::ReaderContext& ctx)
{
    // Reopen the file if necessary
    if (mCtx.filename != ctx.filename)
        openTes4File(ctx.filename);

    // restore group stack, etc.
    mReader.restoreContext(ctx); // FIXME: what to do with the result?

    // mCtx.leftFile is the only thing used in the old context.  Strictly speaking, updating it
    // with the correct value is not really necessary since we're not going to load the rest of
    // the file (most likely to load a CELL or LAND then be done with it).
    mCtx.leftFile = mReader.getFileSize() - mReader.getFileOffset();
    //std::cout << "restore leftFile: " << mCtx.leftFile << std::endl; // FIXME: debug
}

// callback from mReader to ensure hasMoreRecs() can reliably track to EOF
void ESM::ESM4Reader::update(std::size_t size)
{
    mCtx.leftFile -= size;
}
