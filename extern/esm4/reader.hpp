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
#ifndef ESM4_READER_H
#define ESM4_READER_H

#include <vector>
#include <cstddef>

#include <boost/scoped_array.hpp>

#include <OgreDataStream.h>

#include "loadtes4.hpp"
#include "common.hpp"

namespace ESM4
{
    class ReaderObserver
    {
    public:
        ReaderObserver() {}
        virtual ~ReaderObserver() {}

        virtual void update(std::size_t size) = 0;
    };

    typedef std::vector<std::pair<const ESM4::GroupTypeHeader, std::uint32_t> > GroupStack;

    class Reader
    {
        ReaderObserver *mObserver;        // observer for tracking bytes read

        Header          mHeader;          // ESM header // FIXME
        RecordHeader    mRecordHeader;    // header of the current record or group being processed
        SubRecordHeader mSubRecordHeader; // header of the current sub record being processed
        GroupStack      mGroupStack;      // keep track of bytes left to find when a group is done
        std::size_t     mEndOfRecord;     // number of bytes read by sub records
        CellGrid        mCurrCell;        // TODO: should keep keep a map of cell formids // FIXME
        bool            mCellGridValid;
        std::size_t     mRecHeaderSize;

        // TODO: try fixed size buffers on the stack for both below (may be faster)
        boost::scoped_array<unsigned char> mInBuf;
        boost::scoped_array<unsigned char> mDataBuf; // avoid memory leak due to exceptions, etc

        Ogre::DataStreamPtr mStream;
        Ogre::DataStreamPtr mSavedStream;

    public:

        Reader();
        ~Reader();

        std::size_t openTes4File(Ogre::DataStreamPtr stream, const std::string& name);

        // NOTE: must be set to the correct size before calling getRecordHeader()
        void setRecHeaderSize(const std::size_t size);

        void setEsmVersion(unsigned int version) { mHeader.mData.version.ui = version; }
        unsigned int esmVersion() const { return mHeader.mData.version.ui; }

        // Read 24 bytes of header. The caller can then decide whether to process or skip the data.
        bool getRecordHeader();

        inline const RecordHeader& hdr() const { return mRecordHeader; }

        const GroupTypeHeader& grp(std::size_t pos = 0) const;

        // Maybe should throw an exception if called when not valid?
        const CellGrid& currCell() const;
// if performance becomes an issue
#if 0
        inline const CellGrid& currCell() const {
            assert(mCellGridValid && "Attempt to use an invalid cell grid");
            return mCurrCell;
        }
#endif

        // This is set while loading a CELL record (XCLC sub record) and cleared
        // when entering an exterior cell group.
        inline void setCurrCell(const CellGrid& currCell) {
            mCellGridValid = true;
            mCurrCell = currCell;
        }

        // Get the data part of a record
        // Note: assumes the header was read correctly and nothing else was read
        void getRecordData();

        // Skip the data part of a record
        // Note: assumes the header was read correctly and nothing else was read
        void skipRecordData();

        // Skip the rest of the group
        // Note: assumes the header was read correctly and nothing else was read
        void skipGroup();

        // Read 6 bytes of header. The caller can then decide whether to process or skip the data.
        bool getSubRecordHeader();

        inline const SubRecordHeader& subRecordHeader() const { return mSubRecordHeader; }

        // Skip the data part of a subrecord
        // Note: assumes the header was read correctly and nothing else was read
        void skipSubRecordData();

        // Special for a subrecord following a XXXX subrecord
        void skipSubRecordData(std::uint32_t size);

        // Get a subrecord of a particular type and data type
        template<typename T>
        bool getSubRecord(const ESM4::SubRecordTypes type, T& t)
        {
            ESM4::SubRecordHeader hdr;
            if (!get(hdr) || (hdr.typeId != type) || (hdr.dataSize != sizeof(T)))
                return false;

            return get(t);
        }

        template<typename T>
        inline bool get(T& t) {
            return mStream->read(&t, sizeof(T)) == sizeof(T);
        }

        // for arrays
        inline bool get(void* p, std::size_t size) {
            return mStream->read(p, size) == size;
        }

        // Note: does not convert to UTF8
        bool getZString(std::string& str, std::uint16_t size);

        void checkGroupStatus();

        void saveGroupStatus(const ESM4::RecordHeader& hdr);

        void registerForUpdates(ReaderObserver *observer);

        // for debugging only
        size_t stackSize() const { return mGroupStack.size(); }
    };
}

#endif // ESM4_READER_H
