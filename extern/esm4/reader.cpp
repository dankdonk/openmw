/*
  Copyright (C) 2015 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

*/
#include "reader.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif
#include <stdexcept>
#include <cassert>

#include <iostream> // FIXME: debugging only

#include <zlib.h>

#include <components/esm/esm4reader.hpp>

ESM4::Reader::Reader()
: mObserver(nullptr), mEndOfRecord(0), mCellGridValid(false), mRecHeaderSize(sizeof(ESM4::RecordHeader))
{
    mInBuf.reset();
    mDataBuf.reset();
    mStream.setNull();
    mSavedStream.setNull();
}

ESM4::Reader::~Reader()
{
}

std::size_t ESM4::Reader::openTes4File(Ogre::DataStreamPtr stream, const std::string& name)
{
    mStream = stream;
    return mStream->size();
}

void ESM4::Reader::setRecHeaderSize(const std::size_t size)
{
    mRecHeaderSize = size;
}

void ESM4::Reader::registerForUpdates(ESM4::ReaderObserver *observer)
{
    mObserver = observer;
}

bool ESM4::Reader::getRecordHeader()
{
    // FIXME: this seems hacky
    if (/*mStream->eof() && */!mSavedStream.isNull())
    {
        mStream = mSavedStream;
        mSavedStream.setNull();
    }

    // keep track of data left to read from the file
    mObserver->update(mRecHeaderSize);

    return (mStream->read(&mRecordHeader, mRecHeaderSize) == mRecHeaderSize
            && (mEndOfRecord = mStream->tell() + mRecordHeader.record.dataSize)); // for keeping track of sub records
}

bool ESM4::Reader::getSubRecordHeader()
{
    return (mStream->tell() < mEndOfRecord) && get(mSubRecordHeader);
}

void ESM4::Reader::saveGroupStatus(const ESM4::RecordHeader& hdr)
{
//#if 0
    std::string padding = ""; // FIXME: debugging only
    padding.insert(0, mGroupStack.size()*2, ' ');
    std::cout << padding << "Starting record group " << ESM4::printLabel(hdr.group.label, hdr.group.type) << std::endl;
//#endif
    if (hdr.group.groupSize == (std::uint32_t)mRecHeaderSize)
    {
//#if 0
        std::cout << padding << "Igorning record group " // FIXME: debugging only
            << ESM4::printLabel(hdr.group.label, hdr.group.type) << " (empty)" << std::endl;
//#endif
        // don't put on the stack, checkGroupStatus() may not get called before recursing into this method
        mGroupStack.back().second -= hdr.group.groupSize;
        checkGroupStatus();
        return;
    }

    // push group
    mGroupStack.push_back(std::make_pair(hdr.group, hdr.group.groupSize - (std::uint32_t)mRecHeaderSize));

    mCellGridValid = false; // FIXME: is there a better place to set this?
}

const ESM4::CellGrid& ESM4::Reader::currCell() const
{
    // Maybe should throw an exception instead?
    assert(mCellGridValid && "Attempt to use an invalid cell grid");

    return mCurrCell;
}

void ESM4::Reader::checkGroupStatus()
{
    // pop finished groups
    while (!mGroupStack.empty() && mGroupStack.back().second == 0)
    {
        ESM4::GroupTypeHeader grp = mGroupStack.back().first; // FIXME: debugging only
        uint32_t groupSize = mGroupStack.back().first.groupSize;
        mGroupStack.pop_back();
//#if 0
        std::string padding = ""; // FIXME: debugging only
        padding.insert(0, mGroupStack.size()*2, ' ');
        std::cout << padding << "Finished record group " << ESM4::printLabel(grp.label, grp.type) << std::endl;
//#endif
        // Check if the previous group was the final one
        if (mGroupStack.empty())
            return;

        assert (mGroupStack.back().second >= groupSize && "Read more records than available");
#if 0
        if (mGroupStack.back().second < groupSize) // FIXME: debugging only
            std::cerr << ESM4::printLabel(mGroupStack.back().first.label, mGroupStack.back().first.type)
                      << " read more records than available" << std::endl;
#endif
        mGroupStack.back().second -= groupSize;
    }
}

const ESM4::GroupTypeHeader& ESM4::Reader::grp(std::size_t pos) const
{
    assert(pos <= mGroupStack.size()-1 && "ESM4::Reader::grp - exceeded stack depth");

    return (*(mGroupStack.end()-pos-1)).first;
}

void ESM4::Reader::getRecordData()
{
    std::uint32_t bufSize = 0;

    if ((mRecordHeader.record.flags & ESM4::Rec_Compressed) != 0)
    {
        mInBuf.reset(new unsigned char[mRecordHeader.record.dataSize-(int)sizeof(bufSize)]);
        mStream->read(&bufSize, sizeof(bufSize));
        mStream->read(mInBuf.get(), mRecordHeader.record.dataSize-(int)sizeof(bufSize));
        mDataBuf.reset(new unsigned char[bufSize]);

        int ret;
        z_stream strm;
        strm.zalloc = Z_NULL;
        strm.zfree  = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = bufSize;
        strm.next_in = mInBuf.get();
        ret = inflateInit(&strm);
        if (ret != Z_OK)
            throw std::runtime_error("ESM4::Reader::getRecordData - inflateInit failed");

        strm.avail_out = bufSize;
        strm.next_out = mDataBuf.get();
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR && "ESM4::Reader::getRecordData - inflate - state clobbered");
        switch (ret)
        {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR; /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(&strm);
            throw std::runtime_error("ESM4::Reader::getRecordData - inflate failed");
        }
        assert(ret == Z_OK || ret == Z_STREAM_END);

    // For debugging only
#if 0
        std::ostringstream ss;
        for (unsigned int i = 0; i < bufSize; ++i)
        {
            if (mDataBuf[i] > 64 && mDataBuf[i] < 91)
                ss << (char)(mDataBuf[i]) << " ";
            else
                ss << std::setfill('0') << std::setw(2) << std::hex << (int)(mDataBuf[i]);
            if ((i & 0x000f) == 0xf)
                ss << "\n";
            else if (i < bufSize-1)
                ss << " ";
        }
        std::cout << ss.str() << std::endl;
#endif
        inflateEnd(&strm);

        mSavedStream = mStream;
        mStream = Ogre::DataStreamPtr(new Ogre::MemoryDataStream(mDataBuf.get(), bufSize, false, true));
    }

    // keep track of data left to read from the current group
    assert (!mGroupStack.empty() && "Read data for a record without a group");
    mGroupStack.back().second -= (std::uint32_t)mRecHeaderSize + mRecordHeader.record.dataSize;

    // keep track of data left to read from the file
    mObserver->update(mRecordHeader.record.dataSize);

    //std::cout << "data size 0x" << std::hex << mRecordHeader.record.dataSize << std::endl; // FIXME
}

// FIXME: how to without using a temp buffer?
bool ESM4::Reader::getZString(std::string& str, std::uint16_t size)
{
    boost::scoped_array<char> buf(new char[size]);
    if (mStream->read(buf.get(), size) == (size_t)size)
    {
        if (buf[size-1] != 0)
            std::cerr << "ESM4::Reader - string is not terminated with a zero" << std::endl;

        str.assign(buf.get(), size-1); // don't copy null terminator
        return true;
    }
    else
    {
        str.clear();
        return false;
    }
}

void ESM4::Reader::skipGroup()
{
    std::string padding = ""; // FIXME: debugging only
    padding.insert(0, mGroupStack.size()*2, ' ');
    std::cout << padding << "Skipping record group "
              << ESM4::printLabel(mRecordHeader.group.label, mRecordHeader.group.type) << std::endl;

    // Note: subtract the size of header already read before skipping
    mStream->skip(mRecordHeader.group.groupSize - (std::uint32_t)mRecHeaderSize);

    // keep track of data left to read from the file
    mObserver->update((std::size_t)mRecordHeader.group.groupSize - mRecHeaderSize);
}

void ESM4::Reader::skipRecordData()
{
    mStream->skip(mRecordHeader.record.dataSize);

    // keep track of data left to read from the current group
    assert (!mGroupStack.empty() && "Skipping a record without a group");
    mGroupStack.back().second -= (std::uint32_t)mRecHeaderSize + mRecordHeader.record.dataSize;

    // keep track of data left to read from the file
    mObserver->update(mRecordHeader.record.dataSize);
}

void ESM4::Reader::skipSubRecordData()
{
    mStream->skip(mSubRecordHeader.dataSize);
}

void ESM4::Reader::skipSubRecordData(std::uint32_t size)
{
    mStream->skip(size);
}
