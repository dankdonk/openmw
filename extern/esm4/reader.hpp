/*
  Copyright (C) 2015, 2016 cc9cii

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

#include "common.hpp"
#include "tes4.hpp"

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

        std::uint32_t   mModIndex;        // 0x00 reserved, 0xFF in-game
        Header          mHeader;          // ESM header // FIXME
        RecordHeader    mRecordHeader;    // header of the current record or group being processed
        SubRecordHeader mSubRecordHeader; // header of the current sub record being processed
        GroupStack      mGroupStack;      // keep track of bytes left to find when a group is done
        std::size_t     mEndOfRecord;     // number of bytes read by sub records

        CellGrid        mCurrCellGrid;    // TODO: should keep keep a map of cell formids // FIXME
        bool            mCellGridValid;
        FormId          mCurrWorld;       // formId of current world - for grouping CELL records
        FormId          mCurrCell;        // formId of current cell

        std::size_t     mRecHeaderSize;   // default = TES5 size, reduced by setRecHeaderSize()

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

        void loadHeader() { mHeader.load(*this); }
        unsigned int esmVersion() const { return mHeader.mData.version.ui; }
        unsigned int numRecords() const { return mHeader.mData.records; }

        // Read 24 bytes of header. The caller can then decide whether to process or skip the data.
        bool getRecordHeader();

        inline const RecordHeader& hdr() const { return mRecordHeader; }

        const GroupTypeHeader& grp(std::size_t pos = 0) const;

        // FIXME; should this be in the header, or even the back of the vector?
        void setModIndex(int index) { mModIndex = (index << 24) & 0xff000000; }
        void updateModIndicies(const std::vector<std::string>& files);

        // Maybe should throw an exception if called when not valid?
        const CellGrid& currCellGrid() const;

        inline const bool hasCellGrid() const { return mCellGridValid; }

        // This is set while loading a CELL record (XCLC sub record) and invalidated
        // each time loading a CELL (see clearCellGrid())
        inline void setCurrCell(const CellGrid& currCell) {
            mCellGridValid = true;
            mCurrCellGrid = currCell;
        }

        // FIXME: This is called each time a new CELL record is read.  Rather than calling this
        // methos explicitly, mCellGridValid should be set automatically somehow.
        //
        // Cell 2c143 is loaded immedicatly after 1bdb1 and can mistakely appear to have grid 0, 1.
        inline void clearCellGrid() { mCellGridValid = false; }

        // set at the beginning of a CELL load
        inline void setCurrCell(FormId formId) { mCurrCell = formId; }

        inline const FormId currCell() { return mCurrCell; }

        inline void setCurrWorld(FormId formId) { mCurrWorld = formId; }

        inline const FormId currWorld() { return mCurrWorld; }

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

        // Modindex adjusted formId for REFR, ACHR, ACRE // FIXME: maybe use below instead?
        // (see http://www.uesp.net/wiki/Tes4Mod:FormID_Fixup)
        inline FormId adjustFormId(const FormId& id) const {
            return  mModIndex | (id & 0xffffff);
        }

        // ModIndex adjusted formId according to master file dependencies
        inline void adjustFormId(FormId& id) {
            if (!mHeader.mModIndicies.empty())
            {
                int index = (id >> 24) & 0xff;
                id = mHeader.mModIndicies[index] | (id & 0x00ffffff);
            }
        }

        inline void adjustGRUPFormId() {
            if (!mHeader.mModIndicies.empty())
            {
                int index = (mRecordHeader.group.label.value >> 24) & 0xff;
                mRecordHeader.group.label.value
                    = mHeader.mModIndicies[index] | (mRecordHeader.group.label.value & 0x00ffffff);
            }
        }

        bool getFormId(FormId& id) {
            if (!get(id))
                return false;

            adjustFormId(id);
            return true;
        }

        // Note: does not convert to UTF8
        // Note: assumes string size from the subrecord header
        bool getZString(std::string& str);

        void checkGroupStatus();

        void saveGroupStatus();

        void registerForUpdates(ReaderObserver *observer);

        // for debugging only
        size_t stackSize() const { return mGroupStack.size(); }
    };
}

#endif // ESM4_READER_H
