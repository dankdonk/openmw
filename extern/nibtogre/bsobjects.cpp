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

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#include "bsobjects.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "nimodel.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

NiBtOgre::NiAdditionalGeometryData::NiAdditionalGeometryData(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    stream->read(mNumVertices);

    std::uint32_t numBlockInfos;
    stream->read(numBlockInfos);
    mBlockInfos.resize(numBlockInfos);
    for (std::size_t i = 0; i < numBlockInfos; ++i)
    {
        AdditionalDataInfo info;

        stream->read(info.dataType);
        stream->read(info.numChannelBytesPerElement);
        stream->read(info.numChannelBytes);
        stream->read(info.numTotalBytesPerElement);
        stream->read(info.blockIndex);
        stream->read(info.channelOffset);
        stream->read(info.unknown);

        mBlockInfos[i] = std::move(info);
    }

    std::uint32_t numBlocks;
    stream->read(numBlocks);
    mBlocks.resize(numBlocks);
    for (std::size_t i = 0; i < numBlocks; ++i)
    {
        AdditionalDataBlock block;

        stream->read(block.hasData);
        //if (block.hasData)
        {
            stream->read(block.blockSize);
            stream->read(block.numBlocks);
            block.blockOffsets.resize(block.numBlocks);
            for (std::size_t j = 0; j < block.numBlocks; ++j)
            {
                stream->read(block.blockOffsets.at(j));
            }
            stream->read(block.numData);
            block.dataSizes.resize(block.numData);
            for (std::size_t j = 0; j < block.numData; ++j)
            {
                stream->read(block.dataSizes.at(j));
            }

            block.data.resize(block.blockSize);
            for (std::size_t j = 0; j < block.blockSize; ++j)
            {
                stream->read(block.data.at(j));
            }
        }

        mBlocks[i] = std::move(block);
    }
}
