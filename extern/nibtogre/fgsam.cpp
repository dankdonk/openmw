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
#include "fgsam.hpp"

#include <iostream> // FIXME: for debugging only

#include <OgreResourceGroupManager.h> // FIXME: for debugging only
#include <OgreTextureManager.h>       // FIXME: for debugging only
#include <OgreException.h>

namespace NiBtOgre
{
    std::map<std::string, std::unique_ptr<FgSam::FgFiles> > FgSam::sFgFilesMap;

    // FIXME: there aren't always 1:1 base name match
    //        e.g. there is no emperor.tri but emperor.egm exists
    FgSam::FgFiles::FgFiles(const std::string& base)
        : tri(base+"tri"), egm(base+"egm", tri), egt(base+"egt")
    {
        //NiBtOgre::FgCtl ctl("facegen\\si.ctl", "General");
        //ctl.loadImpl();
    }

    FgSam::FgSam()
    {
    }

    FgSam::~FgSam()
    {
    }

    const FgSam::FgFiles *FgSam::getMorphedVertices(std::vector<Ogre::Vector3>& fgVertices,
                                    const std::string& mesh,
                                    const std::vector<float>& raceSymCoeff,
                                    const std::vector<float>& raceAsymCoeff,
                                    const std::vector<float>& npcSymCoeff,
                                    const std::vector<float>& npcAsymCoeff) const
    {
        const FgFiles *fgFiles = getOrLoadByName(mesh);

        if (fgFiles == nullptr)
            return fgFiles;

        // FIXME: Not sure what to do with "Stat Morph Vertices". (see TRI in "fileformats")
        //        For headhuman.nif, the number of vertices are 0x4FB, which equals tri.numVertices(),
        //        and the number of morph vertices are 0x1CD.
        std::size_t numVertices = fgFiles->tri.numVertices();
        std::size_t numMorphVertices = fgFiles->tri.numMorphVertices();

        fgVertices.resize(numVertices);

        const boost::scoped_array<float>& symMorphModeScales = fgFiles->egm.symMorphModeScales();
        const boost::scoped_array<float>& asymMorphModeScales = fgFiles->egm.asymMorphModeScales();
        const boost::scoped_array<std::int16_t>& symMorphModes = fgFiles->egm.symMorphModes();
        const boost::scoped_array<std::int16_t>& asymMorphModes = fgFiles->egm.asymMorphModes();

        const boost::scoped_array<float>& vertices = fgFiles->tri.vertices();

        std::size_t index;
        float coeff, scale, xMorph, yMorph, zMorph;
        for (std::size_t i = 0; i < numVertices; ++i)
        {
            xMorph = yMorph = zMorph = 0.f;
            for (std::size_t j = 0; j < 50; ++j) // for TES4 always 50 sym modes
            {
                // NOTE: just guessed that the race and npc coefficients should be added
                //       (see Thoronir-without-race-coeff.png, looks bad without the race coefficients)
                // TODO: try multiplying?
                coeff = raceSymCoeff[j] + npcSymCoeff[j];
                scale = symMorphModeScales[j];
                // source has all the vertices for a given mode in a group
                index = 3 * ((numVertices + numMorphVertices) * j + i);

                xMorph += coeff * scale * symMorphModes[index + 0];
                yMorph += coeff * scale * symMorphModes[index + 1];
                zMorph += coeff * scale * symMorphModes[index + 2];
            }

            for (std::size_t k = 0; k < 30; ++k) // for TES4 always 30 asym modes
            {
                // WARN: coeff, scale and index reused
                coeff = raceAsymCoeff[k] + npcAsymCoeff[k];
                scale = asymMorphModeScales[k];
                index = 3 * ((numVertices + numMorphVertices) * k + i);

                xMorph += coeff * scale * asymMorphModes[index + 0];
                yMorph += coeff * scale * asymMorphModes[index + 1];
                zMorph += coeff * scale * asymMorphModes[index + 2];
            }

            index = 3 * i; // WARN: index reused
            fgVertices[i].x = vertices[index + 0] + xMorph;
            fgVertices[i].y = vertices[index + 1] + yMorph;
            fgVertices[i].z = vertices[index + 2] + zMorph;
        }

        return fgFiles;
    }

    const FgSam::FgFiles *FgSam::getMorphedTextures(Ogre::TexturePtr fgTexture,
                                    const std::string& mesh,
                                    const std::string& npcName, // FIXME: for debugging only
                                    const std::vector<float>& raceSymCoeff,
                                    const std::vector<float>& raceAsymCoeff,
                                    const std::vector<float>& npcSymCoeff,
                                    const std::vector<float>& npcAsymCoeff) const
    {
        const FgFiles *fgFiles = getOrLoadByName(mesh);

        if (fgFiles == nullptr)
            return fgFiles;

        return getMorphedTextures(fgTexture, fgFiles, npcName, raceSymCoeff, raceAsymCoeff, npcSymCoeff, npcAsymCoeff);
    }

    const FgSam::FgFiles *FgSam::getMorphedTextures(Ogre::TexturePtr fgTexture,
                                    const FgFiles *fgFiles,
                                    const std::string& npcName, // FIXME: for debugging only
                                    const std::vector<float>& raceSymCoeff,
                                    const std::vector<float>& raceAsymCoeff,
                                    const std::vector<float>& npcSymCoeff,
                                    const std::vector<float>& npcAsymCoeff) const
    {
//      // based on http://wiki.ogre3d.org/Creating+dynamic+textures
//      Ogre::TexturePtr texFg = Ogre::TextureManager::getSingleton().getByName(
//             "FaceGen"+npcName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
//      if (texFg.isNull())
//      {
//          texFg = Ogre::TextureManager::getSingleton().createManual(
//              "FaceGen"+npcName, // name
//              Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
//              Ogre::TEX_TYPE_2D, // type
//              fgFiles->egt.numRows(), fgFiles->egt.numColumns(), // width & height
//              0,                 // number of mipmaps; FIXME: should be 2? or 1?
//              Ogre::PF_BYTE_RGBA,
//              Ogre::TU_DEFAULT); // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
//                                 // textures updated very often (e.g. each frame)
//      }
        return nullptr;
    }

    const FgSam::FgFiles *FgSam::getOrLoadByName(const std::string& mesh) const
    {
        std::map<std::string, std::unique_ptr<FgFiles> >::iterator lb = sFgFilesMap.lower_bound(mesh);
        if (lb != sFgFilesMap.end() && !(sFgFilesMap.key_comp()(mesh, lb->first)))
        {
            return lb->second.get();
        }
        else // none found, create one
        {
            size_t pos = mesh.find_last_of("nif");
            if (pos == std::string::npos)
            {
                // FIXME: convert to lowercase instead?
                pos = mesh.find_last_of("NIF");
                if (pos == std::string::npos)
                    return nullptr;
            }

            try
            {
                std::unique_ptr<FgFiles> fgFiles = std::make_unique<FgFiles>(mesh.substr(0, pos - 2));

                lb = sFgFilesMap.insert(lb,
                        std::map<std::string, std::unique_ptr<FgFiles> >::value_type(mesh, std::move(fgFiles)));

                return lb->second.get();
            }
            catch (Ogre::Exception e)
            {
                // most likely resource not found by Ogre
                std::cerr << "FgSam: " << e.getFullDescription() << std::endl;

                return nullptr;
            }
        }
    }
}
