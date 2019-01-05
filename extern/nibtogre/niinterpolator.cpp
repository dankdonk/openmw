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

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#include "niinterpolator.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "ninode.hpp" // static_cast NiNode
#include "nimodel.hpp"
#include "nidata.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

NiBtOgre::NiInterpolator::NiInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiObject(index, stream, model, data)
{
}

const NiBtOgre::KeyGroup<float> *NiBtOgre::NiInterpolator::getMorphKeyGroup()
{
    return nullptr;
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiBSplineInterpolator::NiBSplineInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream.read(mStartTime);
    stream.read(mStopTime);
    stream.read(mSplineDataIndex);
    stream.read(mBasisDataIndex);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiBSplinePoint3Interpolator::NiBSplinePoint3Interpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiBSplineInterpolator(index, stream, model, data)
{
    stream.read(mUnknown1);
    stream.read(mUnknown2);
    stream.read(mUnknown3);
    stream.read(mUnknown4);
    stream.read(mUnknown5);
    stream.read(mUnknown6);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSplineTransformInterpolator::NiBSplineTransformInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiBSplineInterpolator(index, stream, model, data)
{
    stream.read(mTranslation);
    stream.read(mRotation);
    stream.read(mScale);
    stream.read(mTranslationOffset);
    stream.read(mRotationOffset);
    stream.read(mScaleOffset);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSplineCompTransformInterpolator::NiBSplineCompTransformInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiBSplineTransformInterpolator(index, stream, model, data)
{
    stream.read(mTranslationBias);
    stream.read(mTranslationMultiplier);
    stream.read(mRotationBias);
    stream.read(mRotationMultiplier);
    stream.read(mScaleBias);
    stream.read(mScaleMultiplier);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5 (????)
NiBtOgre::NiBlendInterpolator::NiBlendInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream.read(mUnknownShort);
    stream.read(mUnknownInt);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBlendBoolInterpolator::NiBlendBoolInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiBlendInterpolator(index, stream, model, data)
{
    stream.read(mValue);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBlendFloatInterpolator::NiBlendFloatInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiBlendInterpolator(index, stream, model, data)
{
    stream.read(mValue);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBlendPoint3Interpolator::NiBlendPoint3Interpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiBlendInterpolator(index, stream, model, data)
{
    stream.read(mValue);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBoolInterpolator::NiBoolInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream.read(mBoolalue);
    stream.read(mDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiFloatInterpolator::NiFloatInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream.read(mFloatValue);
    stream.read(mDataIndex);
}

const NiBtOgre::KeyGroup<float> *NiBtOgre::NiFloatInterpolator::getMorphKeyGroup()
{
    if (mDataIndex == -1)
        return nullptr;

    return &mModel.getRef<NiFloatData>(mDataIndex)->mData;
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPathInterpolator::NiPathInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream.skip(sizeof(std::uint16_t));
    stream.skip(sizeof(std::uint32_t));
    stream.skip(sizeof(float));
    stream.skip(sizeof(float));
    stream.skip(sizeof(std::uint16_t));

    stream.read(mPosDataIndex);
    stream.read(mFloatDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPoint3Interpolator::NiPoint3Interpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream.read(mPoint3Value);
    stream.read(mDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiTransformInterpolator::NiTransformInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream.read(mTranslation);
    stream.read(mRotation);
    stream.read(mScale);
    stream.read(mDataIndex);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiLookAtInterpolator::NiLookAtInterpolator(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream.read(mUnknown);

    //stream.getPtr<NiNode>(mLookAt, model.objects());
    std::int32_t rIndex = -1;
    stream.read(rIndex);
    mLookAt = model.getRef<NiNode>(rIndex);

    stream.readLongString(mTarget);
    if (stream.nifVer() <= 0x14050000) // up to 20.5.0.0
    {
        stream.read(mTranslation);
        stream.read(mRotation);
        stream.read(mScale);
    }
    stream.read(mUnknownLink1);
    stream.read(mUnknownLink2);
    stream.read(mUnknownLink3);
}
