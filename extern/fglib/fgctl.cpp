/*
  Copyright (C) 2020 cc9cii

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

  Based on the cryptic notes on https://facegen.com/dl/sdk/doc/manual/indepth.html
  and https://facegen.com/dl/sdk/doc/manual/fileformats.html.

*/
#include "fgctl.hpp"

#include <stdexcept>
#include <iostream> // NOTE: for debugging only

namespace FgLib
{
    FgCtl::FgCtl(const Ogre::String& name/*, const Ogre::String& group*/)
//        : mFgStream(name), mGroup(group), mName(name)
    {
        FgStream ctl(name);

        ctl.read(mFileType);             // FIXME: assert that it is "FRCTL001"
        ctl.read(mGeometryBasisVersion); // FIXME: assert that it is the same as EGM
        ctl.read(mTextureBasisVersion);  // FIXME: assert that it is the same as EGT

        ctl.read(mNumSymMorphModes);
        if (mNumSymMorphModes != 50)
            throw std::runtime_error("Number of Symmetric Morph Modes is not 50");
        ctl.read(mNumAsymMorphModes);
        if (mNumAsymMorphModes != 30)
            throw std::runtime_error("Number of Asymmetric Morph Modes is not 30");
        ctl.read(mNumSymTextureModes);
        if (mNumSymTextureModes != 50)
            throw std::runtime_error("Number of Symmetric Texture Modes is not 50");
        ctl.read(mNumAsymTextureModes);
        if (mNumAsymTextureModes != 0)
            throw std::runtime_error("Number of Asymmetric Texture Modes is not 0");

        readLinearControls(mNumGSLinearControls, 50/*mNumSymMorphModes*/,    mGSymLinearControls,  ctl);
        readLinearControls(mNumGALinearControls, 30/*mNumAsymMorphModes*/,   mGAsymLinearControls, ctl);
        readLinearControls(mNumTSLinearControls, 50/*mNumSymTextureModes*/,  mTSymLinearControls,  ctl);
        readLinearControls(mNumTALinearControls,  0/*mNumAsymTextureModes*/, mTAsymLinearControls, ctl);

        // addr in si.ctl 0x5EC5 at this point

        // mNumSymMorphModes = mNumSymTextureModes = 50
        readOffsetLinearControls(50, mAllPrtOffsetLinearControls,  ctl);
        readOffsetLinearControls(50, mAfroPrtOffsetLinearControls, ctl);
        readOffsetLinearControls(50, mAsiaPrtOffsetLinearControls, ctl);
        readOffsetLinearControls(50, mEindPrtOffsetLinearControls, ctl);
        readOffsetLinearControls(50, mEuroPrtOffsetLinearControls, ctl);

        // TODO: remainder of si.ctl, race morphing offset linear controls, density
        //       distribution for each race group, etc
    }

    FgCtl::~FgCtl()
    {
    }

    void FgCtl::readLinearControls(uint32_t& numControls, uint32_t numModes,
            std::vector<std::pair<std::vector<float>, std::string> >& controls, FgStream& ctl)
    {
        char buf[100]; // 100 should be enough
        std::uint32_t strSize;
        ctl.read(numControls);
        controls.resize(numControls);
        for (std::size_t i = 0; i < numControls; ++i)
        {
            controls[i].first.resize(numModes);
            for (std::size_t j = 0; j < numModes; ++j)
                ctl.read(controls[i].first.at(j));

            ctl.read(strSize);
            //boost::scoped_array<char> buf(new char[strSize+1]);
            for (std::size_t c = 0; c < strSize; ++c)
                ctl.read(buf[c]);
            buf[strSize] = 0x00;
            controls[i].second = std::string(buf);

    // NOTE: for debugging only
    //      std::cout << controls[i].second << std::endl;
    //      for (std::size_t j = 0; j < numModes; ++j)
    //          std::cout << controls[i].first[j] << std::endl;
        }
    }

    void FgCtl::readOffsetLinearControls(uint32_t numModes,
            std::vector<std::pair<std::vector<float>, float> >& controls, FgStream& ctl)
    {
        // guess: 0 = age sym, 1 = age texture, 2 = gender sym, 3 = gender texture
        controls.resize(4);
        for (std::size_t i = 0; i < 4; ++i)
        {
            controls[i].first.resize(numModes);
            for (std::size_t j = 0; j < numModes; ++j)
                ctl.read(controls[i].first.at(j));
            ctl.read(controls[i].second);

    // NOTE: for debugging only
    //      std::cout << "type " << i << std::endl;
    //      float res = 0.f;
    //      for (std::size_t j = 0; j < numModes; ++j)
    //          res += controls[i].first[j];
    //      std::cout << "sum " << res << std::endl;
    //          //std::cout << controls[i].first[j] << std::endl;
    //      //std::cout << "offset " << controls[i].second << std::endl;
        }
    }
#if 0
    // only called if this resource is not being loaded from a ManualResourceLoader
    void FgCtl::loadImpl()
    {
        mFgStream.read(mFileType);             // FIXME: assert that it is "FRCTL001"
        mFgStream.read(mGeometryBasisVersion); // FIXME: assert that it is the same as EGM
        mFgStream.read(mTextureBasisVersion);  // FIXME: assert that it is the same as EGT

        mFgStream.read(mNumSymMorphModes);
        if (mNumSymMorphModes != 50)
            throw std::runtime_error("Number of Symmetric Morph Modes is not 50");
        mFgStream.read(mNumAsymMorphModes);
        if (mNumAsymMorphModes != 30)
            throw std::runtime_error("Number of Asymmetric Morph Modes is not 30");
        mFgStream.read(mNumSymTextureModes);
        if (mNumSymTextureModes != 50)
            throw std::runtime_error("Number of Symmetric Texture Modes is not 50");
        mFgStream.read(mNumAsymTextureModes);
        if (mNumAsymTextureModes != 0)
            throw std::runtime_error("Number of Asymmetric Texture Modes is not 0");

        readLinearControls(mNumGSLinearControls, 50/*mNumSymMorphModes*/,    mGSymLinearControls);
        readLinearControls(mNumGALinearControls, 30/*mNumAsymMorphModes*/,   mGAsymLinearControls);
        readLinearControls(mNumTSLinearControls, 50/*mNumSymTextureModes*/,  mTSymLinearControls);
        readLinearControls(mNumTALinearControls,  0/*mNumAsymTextureModes*/, mTAsymLinearControls);

        // addr in si.ctl 0x5EC5 at this point

        // mNumSymMorphModes = mNumSymTextureModes = 50
        readOffsetLinearControls(50, mAllPrtOffsetLinearControls);
        readOffsetLinearControls(50, mAfroPrtOffsetLinearControls);
        readOffsetLinearControls(50, mAsiaPrtOffsetLinearControls);
        readOffsetLinearControls(50, mEindPrtOffsetLinearControls);
        readOffsetLinearControls(50, mEuroPrtOffsetLinearControls);

        // TODO: remainder of si.ctl, race morphing offset linear controls, density
        //       distribution for each race group, etc
    }

    void FgCtl::readLinearControls(uint32_t& numControls, uint32_t numModes,
            std::vector<std::pair<std::vector<float>, std::string> >& controls)
    {
        char buf[100]; // 100 should be enough
        std::uint32_t strSize;
        mFgStream.read(numControls);
        controls.resize(numControls);
        for (std::size_t i = 0; i < numControls; ++i)
        {
            controls[i].first.resize(numModes);
            for (std::size_t j = 0; j < numModes; ++j)
                mFgStream.read(controls[i].first.at(j));

            mFgStream.read(strSize);
            //boost::scoped_array<char> buf(new char[strSize+1]);
            for (std::size_t c = 0; c < strSize; ++c)
                mFgStream.read(buf[c]);
            buf[strSize] = 0x00;
            controls[i].second = std::string(buf);

    // NOTE: for debugging only
    //      std::cout << controls[i].second << std::endl;
    //      for (std::size_t j = 0; j < numModes; ++j)
    //          std::cout << controls[i].first[j] << std::endl;
        }
    }

    void FgCtl::readOffsetLinearControls(uint32_t numModes,
            std::vector<std::pair<std::vector<float>, float> >& controls)
    {
        // guess: 0 = age sym, 1 = age texture, 2 = gender sym, 3 = gender texture
        controls.resize(4);
        for (std::size_t i = 0; i < 4; ++i)
        {
            controls[i].first.resize(numModes);
            for (std::size_t j = 0; j < numModes; ++j)
                mFgStream.read(controls[i].first.at(j));
            mFgStream.read(controls[i].second);

    // NOTE: for debugging only
    //      std::cout << "type " << i << std::endl;
    //      float res = 0.f;
    //      for (std::size_t j = 0; j < numModes; ++j)
    //          res += controls[i].first[j];
    //      std::cout << "sum " << res << std::endl;
    //          //std::cout << controls[i].first[j] << std::endl;
    //      //std::cout << "offset " << controls[i].second << std::endl;
        }
    }
#endif
}
