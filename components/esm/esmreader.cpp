#include "esmreader.hpp"
#include <stdexcept>

#include "../files/constrainedfiledatastream.hpp"

namespace ESM
{

using namespace Misc;

    std::string ESMReader::getName() const
    {
        return mCtx.filename;
    }

ESM_Context ESMReader::getContext()
{
    // Update the file position before returning
    mCtx.filePos = mEsm->tell();
    return mCtx;
}

ESMReader::ESMReader()
    : mIdx(0)
    , mRecordFlags(0)
    , mBuffer(50*1024)
    , mGlobalReaderList(NULL)
    , mEncoder(NULL)
{
}

int ESMReader::getFormat() const
{
    return mHeader.mFormat;
}

void ESMReader::restoreContext(const ESM_Context &rc)
{
    // Reopen the file if necessary
    if (mCtx.filename != rc.filename)
        openRaw(rc.filename);

    // Copy the data
    mCtx = rc;

    // Make sure we seek to the right place
    mEsm->seek(mCtx.filePos);
}

void ESMReader::close()
{
    mEsm.reset();
    mCtx.filename.clear();
    mCtx.leftFile = 0;
    mCtx.leftRec = 0;
    mCtx.leftSub = 0;
    mCtx.subCached = false;
    mCtx.recName.val = 0;
    mCtx.subName.val = 0;
}

void ESMReader::openRaw(Ogre::DataStreamPtr _esm, const std::string &name)
{
    close();
    mEsm = _esm;
    mCtx.filename = name;
    mCtx.leftFile = mEsm->size();
}

void ESMReader::open(Ogre::DataStreamPtr _esm, const std::string &name)
{
    openRaw(_esm, name);

    NAME modVer = getRecName();
    if (modVer == "TES3")
    {
        getRecHeader();

        mHeader.load (*this);
    }
    else if (modVer == "TES4")
    {
        mHeader.mData.author.assign("");
        mHeader.mData.desc.assign("");
        char buf[512]; // arbitrary number
        unsigned short size;

        skip(16); // skip the rest of the header, note it may be 4 bytes longer

        NAME rec = getRecName();
        if (rec != "HEDR")
            rec = getRecName(); // adjust for extra 4 bytes
        bool readRec = true;

        while (mEsm->size() - mEsm->tell() >= 4) // Shivering Isle or Bashed Patch can end here
        {
            if (!readRec) // may be already read
                rec = getRecName();
            else
                readRec = false;

            switch (rec.val)
            {
                case 0x52444548: // HEDR
                {
                    skip(2); // data size
                    getT(mHeader.mData.version);
                    getT(mHeader.mData.records);
                    skip(4); // skip next available object id
                    break;
                }
                case 0x4d414e43: // CNAM
                {
                    getT(size);
                    getExact(buf, size);
                    std::string author;
                    size = std::min(size, (unsigned short)32); // clamp for TES3 format
                    author.assign(buf, size - 1); // don't copy null terminator
                    mHeader.mData.author.assign(author);
                    break;
                }
                case 0x4d414e53: // SNAM
                {
                    getT(size);
                    getExact(buf, size);
                    std::string desc;
                    size = std::min(size, (unsigned short)256); // clamp for TES3 format
                    desc.assign(buf, size - 1); // don't copy null terminator
                    mHeader.mData.desc.assign(desc);
                    break;
                }
                case 0x5453414d: // MAST
                {
                    Header::MasterData m;
                    getT(size);
                    getExact(buf, size);
                    m.name.assign(buf, size-1); // don't copy null terminator

                    rec = getRecName();
                    if (rec == "DATA")
                    {
                        getT(size);
                        getT(m.size); // 64 bits
                    }
                    else
                    {
                        // some esp's don't have DATA subrecord
                        m.size = 0;
                        readRec = true; // don't read again at the top of while loop
                    }
                    mHeader.mMaster.push_back (m);
                    break;
                }
                case 0x56544e49: // INTV
                case 0x43434e49: // INCC
                case 0x4d414e4f: // ONAM
                {
                    getT(size);
                    skip(size);
                    break;
                }
                case 0x50555247: // GRUP
                default:
                    return;      // all done
            }
        }
        return;
    }
    else
        fail("Not a valid Morrowind file");
}

void ESMReader::open(const std::string &file)
{
    open (openConstrainedFileDataStream (file.c_str ()), file);
}

void ESMReader::openRaw(const std::string &file)
{
    openRaw (openConstrainedFileDataStream (file.c_str ()), file);
}

int64_t ESMReader::getHNLong(const char *name)
{
    int64_t val;
    getHNT(val, name);
    return val;
}

std::string ESMReader::getHNOString(const char* name)
{
    if (isNextSub(name))
        return getHString();
    return "";
}

std::string ESMReader::getHNString(const char* name)
{
    getSubNameIs(name);
    return getHString();
}

void ESMReader::getHNString(const int name, std::string& str)
{
    getSubNameIs(name);
    getHString(str);
}

std::string ESMReader::getHString()
{
    getSubHeader();

    // Hack to make MultiMark.esp load. Zero-length strings do not
    // occur in any of the official mods, but MultiMark makes use of
    // them. For some reason, they break the rules, and contain a byte
    // (value 0) even if the header says there is no data. If
    // Morrowind accepts it, so should we.
    if (mCtx.leftSub == 0)
    {
        // Skip the following zero byte
        mCtx.leftRec--;
        char c;
        getExact(&c, 1);
        return "";
    }

    return getString(mCtx.leftSub);
}

void ESMReader::getHString(std::string& str)
{
    getSubHeader();

    // Hack to make MultiMark.esp load. Zero-length strings do not
    // occur in any of the official mods, but MultiMark makes use of
    // them. For some reason, they break the rules, and contain a byte
    // (value 0) even if the header says there is no data. If
    // Morrowind accepts it, so should we.
    if (mCtx.leftSub == 0)
    {
        // Skip the following zero byte
        mCtx.leftRec--;
        char c;
        getExact(&c, 1);
        str = "";
        return;
    }

    getString(str, mCtx.leftSub);
}

void ESMReader::getHExact(void*p, int size)
{
    getSubHeader();
    if (size != static_cast<int> (mCtx.leftSub))
    {
        std::stringstream error;
        error << "getHExact(): size mismatch (requested " << size << ", got " << mCtx.leftSub << ")";
        fail(error.str());
    }
    getExact(p, size);
}

// Read the given number of bytes from a named subrecord
void ESMReader::getHNExact(void*p, int size, const char* name)
{
    getSubNameIs(name);
    getHExact(p, size);
}

// Get the next subrecord name and check if it matches the parameter
void ESMReader::getSubNameIs(const char* name)
{
    getSubName();
    if (mCtx.subName != name)
        fail(
                "Expected subrecord " + std::string(name) + " but got "
                        + mCtx.subName.toString());
}

void ESMReader::getSubNameIs(const int name)
{
    getSubName();
    if (mCtx.subName != name)
    {
        unsigned char typeName[4];
        typeName[0] =  name        & 0xff;
        typeName[1] = (name >>  8) & 0xff;
        typeName[2] = (name >> 16) & 0xff;
        typeName[3] = (name >> 24) & 0xff;

        std::string subName = std::string((char*)typeName, 4);

        fail("Expected subrecord " + subName + " but got "
                        + mCtx.subName.toString());
    }
}

bool ESMReader::isNextSub(const char* name)
{
    if (!mCtx.leftRec)
        return false;

    getSubName();

    // If the name didn't match, then mark the it as 'cached' so it's
    // available for the next call to getSubName.
    mCtx.subCached = (mCtx.subName != name);

    // If subCached is false, then subName == name.
    return !mCtx.subCached;
}

bool ESMReader::isNextSub(const int name)
{
    if (!mCtx.leftRec)
        return false;

    getSubName();

    // If the name didn't match, then mark the it as 'cached' so it's
    // available for the next call to getSubName.
    mCtx.subCached = (mCtx.subName != name);

    // If subCached is false, then subName == name.
    return !mCtx.subCached;
}

bool ESMReader::peekNextSub(const char *name)
{
    if (!mCtx.leftRec)
        return false;

    getSubName();

    mCtx.subCached = true;
    return mCtx.subName == name;
}

void ESMReader::cacheSubName()
{
    mCtx.subCached = true;
}

// Read subrecord name. This gets called a LOT, so I've optimized it
// slightly.
void ESMReader::getSubName()
{
    // If the name has already been read, do nothing
    if (mCtx.subCached)
    {
        mCtx.subCached = false;
        return;
    }

    // reading the subrecord data anyway.
    getExact(mCtx.subName.name, 4);
    mCtx.leftRec -= 4;
}

bool ESMReader::isEmptyOrGetName()
{
    if (mCtx.leftRec)
    {
        getExact(mCtx.subName.name, 4);
        mCtx.leftRec -= 4;
        return false;
    }
    return true;
}

void ESMReader::skipHSub()
{
    getSubHeader();
    skip(mCtx.leftSub);
}

void ESMReader::skipHSubSize(int size)
{
    skipHSub();
    if (static_cast<int> (mCtx.leftSub) != size)
        fail("skipHSubSize() mismatch");
}

void ESMReader::skipHSubUntil(const char *name)
{
    while (hasMoreSubs() && !isNextSub(name))
    {
        mCtx.subCached = false;
        skipHSub();
    }
    if (hasMoreSubs())
        mCtx.subCached = true;
}

void ESMReader::getSubHeader()
{
    if (mCtx.leftRec < 4)
        fail("End of record while reading sub-record header");

    // Get subrecord size
    getT(mCtx.leftSub);

    // Adjust number of record bytes left
    mCtx.leftRec -= mCtx.leftSub + 4;
}

void ESMReader::getSubHeaderIs(int size)
{
    getSubHeader();
    if (size != static_cast<int> (mCtx.leftSub))
        fail("getSubHeaderIs(): Sub header mismatch");
}

NAME ESMReader::getRecName()
{
    if (!hasMoreRecs())
        fail("No more records, getRecName() failed");
    getName(mCtx.recName);
    mCtx.leftFile -= 4;

    // Make sure we don't carry over any old cached subrecord
    // names. This can happen in some cases when we skip parts of a
    // record.
    mCtx.subCached = false;

    return mCtx.recName;
}

void ESMReader::skipRecord()
{
    skip(mCtx.leftRec);
    mCtx.leftRec = 0;
    mCtx.subCached = false;
}

void ESMReader::getRecHeader(uint32_t &flags)
{
    // General error checking
    if (mCtx.leftFile < 12)
        fail("End of file while reading record header");
    if (mCtx.leftRec)
        fail("Previous record contains unread bytes");

    getUint(mCtx.leftRec);
    getUint(flags);// This header entry is always zero
    getUint(flags);
    mCtx.leftFile -= 12;

    // Check that sizes add up
    if (mCtx.leftFile < mCtx.leftRec)
        fail("Record size is larger than rest of file");

    // Adjust number of bytes mCtx.left in file
    mCtx.leftFile -= mCtx.leftRec;
}

/*************************************************************************
 *
 *  Lowest level data reading and misc methods
 *
 *************************************************************************/

void ESMReader::getExact(void*x, int size)
{
    try
    {
        int t = mEsm->read(x, size);
        if (t != size)
            fail("Read error");
    }
    catch (std::exception& e)
    {
        fail(std::string("Read error: ") + e.what());
    }
}

std::string ESMReader::getString(int size)
{
    size_t s = size;
    if (mBuffer.size() <= s)
        // Add some extra padding to reduce the chance of having to resize
        // again later.
        mBuffer.resize(3*s);

    // And make sure the string is zero terminated
    mBuffer[s] = 0;

    // read ESM data
    char *ptr = &mBuffer[0];
    getExact(ptr, size);

    size = strnlen(ptr, size);

    // Convert to UTF8 and return
    if (mEncoder)
        return mEncoder->getUtf8(ptr, size);

    return std::string (ptr, size);
}

void ESMReader::getString(std::string& str, int size)
{
    size_t s = size;
    if (mBuffer.size() <= s)
        // Add some extra padding to reduce the chance of having to resize again later.
        mBuffer.resize(3*s);

    mBuffer[s] = 0; // And make sure the string is zero terminated

    char *ptr = &mBuffer[0];
    getExact(ptr, size); // read ESM data

    size = static_cast<int>(strnlen(ptr, size));

    if (mEncoder)
        str = mEncoder->getUtf8(ptr, size); // Convert to UTF8 and return
    else
        str = std::string (ptr, size);
}

void ESMReader::fail(const std::string &msg)
{
    using namespace std;

    stringstream ss;

    ss << "ESM Error: " << msg;
    ss << "\n  File: " << mCtx.filename;
    ss << "\n  Record: " << mCtx.recName.toString();
    ss << "\n  Subrecord: " << mCtx.subName.toString();
    if (mEsm)
        ss << "\n  Offset: 0x" << hex << mEsm->tell();
    throw std::runtime_error(ss.str());
}

void ESMReader::setEncoder(ToUTF8::Utf8Encoder* encoder)
{
    mEncoder = encoder;
}

}
