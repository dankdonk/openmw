/*
  Copyright (C) 2015-2018 cc9cii

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
#ifndef NIBTOGRE_NISTREAM_H
#define NIBTOGRE_NISTREAM_H

#include <vector>
#include <cstddef>

#include <OgreDataStream.h>

class btVector3;

namespace Ogre
{
    class Vector3;
    class Vector4;
    class Matrix3;
}

namespace NiBtOgre
{
    class Header;

    class NiStream
    {
        Ogre::DataStreamPtr mStream;
        Header *mHeader;

        // cache these here for convenient access
        std::uint32_t mVer;
        std::uint32_t mUserVer;
        std::uint32_t mUserVer2;

        // default, copy and assignment not allowed
        NiStream();
        NiStream(const NiStream& other);
        NiStream& operator=(const NiStream& other);

    public:

        NiStream(const std::string& name);
        ~NiStream();

        size_t tell() { return mStream->tell(); } // FIXME: debugging only
        const std::string& getName() { return mStream->getName(); } // FIXME: debugging only

        void setHeader(Header *header);

        inline std::uint32_t nifVer() const { return mVer; }
        inline std::uint32_t userVer() const { return mUserVer; }
        inline std::uint32_t userVer2() const { return mUserVer2; }

        inline void readNifVer(std::uint32_t& value) {
            mStream->read(&mVer, sizeof(std::uint32_t));
            value = mVer;
        }
        inline void readUserVer(std::uint32_t& value) {
            mStream->read(&mUserVer, sizeof(std::uint32_t));
            value = mUserVer;
        }
        inline void readUserVer2(std::uint32_t& value) {
            mStream->read(&mUserVer2, sizeof(std::uint32_t));
            value = mUserVer2;
        }

        std::string readString();
        void readSizedString(std::string& str);
        void readShortString(std::string& str);
        // FIXME: does not check for duplicate strings
        // - can keep a map of indicies but is it worth the trouble to save some memory?
        void readLongString(std::uint32_t& index); // store string in header and set index

        bool getBool();
        void getBool(bool& value);

        void readQuaternionXYZW(Ogre::Quaternion& value);
        void readBtVector3(btVector3& value);

        inline std::string getLine() { return mStream->getLine(); }

        template<typename T>
        inline void read(T& t) {
            mStream->read(&t, sizeof(T));
        }

        template<typename T>
        inline T read() {
            T value;
            mStream->read(&value, sizeof(T));
            return value;
        }

        inline void skip(unsigned long length) { mStream->skip(length); }

        template<typename T>
        inline void readVector(std::vector<T>& data, std::uint32_t size) {
            data.resize(size);
            for (std::uint32_t i = 0; i < size; ++i)
                read(data.at(i));
        }

        template<typename T>
        inline void readVector(std::vector<T>& data) {
            std::uint32_t size = 0;
            mStream->read(&size, sizeof(std::uint32_t));

            data.resize(size);
            for (std::uint32_t i = 0; i < size; ++i)
                read(data.at(i));
        }
    };

    template<>
    void NiStream::read<btVector3>(btVector3& value);

    template<>
    void NiStream::read<Ogre::Vector3>(Ogre::Vector3& value);

    template<>
    void NiStream::read<Ogre::Vector4>(Ogre::Vector4& value);

    template<>
    void NiStream::read<Ogre::Matrix3>(Ogre::Matrix3& value);

    template<>
    void NiStream::read<Ogre::Quaternion>(Ogre::Quaternion& value);
}

#endif // NIBTOGRE_NISTREAM_H
