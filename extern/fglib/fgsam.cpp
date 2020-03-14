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

#include <stdexcept>
#include <iostream> // FIXME: for debugging only

#include <boost/algorithm/string.hpp>

#include <OgrePixelFormat.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreCommon.h> // Ogre::Box
#include <OgreTextureManager.h>       // FIXME: for debugging only

#include <extern/nibtogre/nimodel.hpp>

#include "fgfile.hpp"
#include "fgtri.hpp"
#include "fgegm.hpp"
#include "fgegt.hpp"
#include "fgctl.hpp"

namespace FgLib
{
    bool FgSam::buildMorphedVertices(NiBtOgre::NiModel *model,
                                   const std::string& nif,
                                   const std::vector<float>& raceSymCoeff,
                                   const std::vector<float>& raceAsymCoeff,
                                   const std::vector<float>& npcSymCoeff,
                                   const std::vector<float>& npcAsymCoeff) const
    {
        FgFile<FgTri> triFile;
        const FgTri *tri = triFile.getOrLoadByName(nif);

        if (tri == nullptr)
            return false; // not possible to recover

        FgFile<FgEgm> egmFile;
        const FgEgm *egm = egmFile.getOrLoadByName(nif);

        if (egm == nullptr)
            return false; // not possible to recover

        // NOTE: Not sure what to do with "Stat Morph Vertices". (see TRI in "fileformats")
        //       For headhuman.nif, the number of vertices are 0x4FB, which equals tri.numVertices(),
        //       and the number of morph vertices are 0x1CD.
        //       (these are most likely used for poses such as lip sync and facial expressions)
        std::size_t numVertices = tri->numVertices();
        std::size_t numMorphVertices = tri->numMorphVertices();

        // TODO: maybe just return false rather than throw?
        if (egm->numVertices() != (numVertices + numMorphVertices))
            throw std::runtime_error("SAM: Number of EGM vertices does not match that of TRI");

        // WARN: throws if more than one NiTriBasedGeom in the model
        std::vector<Ogre::Vector3>& fgMorphVertices = model->fgMorphVertices();
        fgMorphVertices.resize(numVertices);

        const boost::scoped_array<float>& symMorphModeScales = egm->symMorphModeScales();
        const boost::scoped_array<float>& asymMorphModeScales = egm->asymMorphModeScales();
        const boost::scoped_array<std::int16_t>& symMorphModes = egm->symMorphModes();
        const boost::scoped_array<std::int16_t>& asymMorphModes = egm->asymMorphModes();

        const boost::scoped_array<float>& vertices = tri->vertices();

        std::size_t index;
        float coeff, scale, xMorph, yMorph, zMorph;
        for (std::size_t i = 0; i < numVertices; ++i)
        {
            xMorph = yMorph = zMorph = 0.f;
            for (std::size_t j = 0; j < 50; ++j) // for TES4 always 50 sym modes
            {
                // NOTE: just guessed that the race and npc coefficients should be added
                //       (see Thoronir-without-race-coeff.png, looks bad without the race coefficients)
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
            fgMorphVertices[i].x = vertices[index + 0] + xMorph;
            fgMorphVertices[i].y = vertices[index + 1] + yMorph;
            fgMorphVertices[i].z = vertices[index + 2] + zMorph;
        }

        // FIXME: update normals, tangents and bitangents?

        model->useFgMorphVertices();

        return true;
    }

    // derive the age of the NPC
    float FgSam::getAge(const std::vector<float>& raceSymCoeff, const std::vector<float>& npcSymCoeff) const
    {
        FgFile<FgCtl> ctlFile;
        const FgCtl *ctl = ctlFile.getOrLoadByName("facegen\\si.ctl"); // WARN: hard-coded path
        const std::vector<std::pair<std::vector<float>, float> >& allPrtOffsetLinearControls
            = ctl->allPrtOffsetCtl();

        float coeff, scale;
        float age   = 0.f;
        for (std::size_t j = 0; j < 50; ++j)
        {
            coeff = raceSymCoeff[j] + npcSymCoeff[j];
            scale = allPrtOffsetLinearControls[0/*age GS*/].first[j];

            age += coeff * scale;
        }

        return age + allPrtOffsetLinearControls[0/*age GS*/].second;
    }

    // returns the filename of the age based detail texture (from which the normal texture can
    // then be derived)
    //
    // FIXME: not sure if we're meant to interpolate the textures or simply pick the bottom one
    // TODO: choose the closest one instead?   i.e. if age is 56 choose 60 rather than 50
    std::string FgSam::getHeadHumanDetailTexture(const std::string& mesh, float age, bool isFemale) const
    {
        // TODO: should scan the Textures\Characters\Imperial directory and get the list of
        //       detail textures rather than hard coding the age 10 them here
        // NecromancerMaleHighElf4 has age of 8.7744 in the construction set
        if (age < 10)
            age = 10;
            //throw std::runtime_error("SAM: NPC age less than 10");

        std::size_t pos = mesh.find_last_of(".");
        if (pos == std::string::npos)
            return "";

        int val = int(age) / 10;
        std::string texture = "textures" + mesh.substr(6, pos - 6); // 6 to take away "meshes"
        texture += (isFemale ? "f" : "m") + std::to_string(val) + "0.dds";

        return texture;
    }

    std::string FgSam::getNpcDetailTexture_0(const std::string& npcFormIdString) const
    {
        // FIXME: need to be able to check other than oblivion.esm - can this hard-coded path change?
        std::string textureFile = "textures\\faces\\oblivion.esm\\"+npcFormIdString+"_0.dds";
        if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(textureFile))
            return textureFile;

        // mName = "textures\\faces\\oblivion.esm\\0001A117_0.dds"
        // SEBelmyneDreleth does not have a facegen texture
        return "";
    }

    bool FgSam::getMorphedTexture(Ogre::TexturePtr& morphTexture,
                                    const std::string& mesh,
                                    const std::string& texture,
                                    const std::string& npcName,
                                    const std::vector<float>& raceSymCoeff,
                                    const std::vector<float>& npcSymCoeff) const
    {
        FgFile<FgEgt> egtFile;
        const FgEgt *egt = egtFile.getOrLoadByName(mesh);

        if (egt == nullptr)
            return false; // texture morph not possible

        return getMorphedTexture(morphTexture, egt, texture, npcName, raceSymCoeff, npcSymCoeff);
    }

    bool FgSam::getMorphedBodyTexture(Ogre::TexturePtr& morphTexture,
                                    const std::string& mesh,
                                    const std::string& texture,
                                    const std::string& npcName,
                                    const std::vector<float>& raceSymCoeff,
                                    const std::vector<float>& npcSymCoeff) const
    {
        std::string meshName = boost::to_lower_copy(mesh);
        //bool isFemale = meshName.find("female") != std::string::npos;

        // femaleupperbody.nif -> upperbodyhumanfemale.egt
        // femalelowerbody.nif -> body.egt
        // upperbody.nif       -> upperbodyhumanmale.egt
        // lowerbody.nif       -> body.egt
        std::size_t pos = mesh.find_last_of(".");
        if (pos == std::string::npos)
            return false; // should throw?

        if (meshName.find("femaleupperbody") != std::string::npos)
            meshName = mesh.substr(0, pos-15) + "upperbodyhumanfemale.nif";
        else if (meshName.find("femalelowerbody") != std::string::npos)
            meshName = mesh.substr(0, pos-15) + "body.nif";
        else if (meshName.find("upperbody") != std::string::npos)
            meshName = mesh.substr(0, pos-9) + "upperbodyhumanmale.nif";
        else if (meshName.find("lowerbody") != std::string::npos)
            meshName = mesh.substr(0, pos-9) + "body.nif";
        else
            return false; // should throw?

        FgFile<FgEgt> egtFile;
        const FgEgt *egt = egtFile.getOrLoadByName(meshName);

        if (egt == nullptr)
            return false; // texture morph not possible

        return getMorphedTexture(morphTexture, egt, texture, npcName, raceSymCoeff, npcSymCoeff);
    }

    bool FgSam::getMorphedTexture(Ogre::TexturePtr& morphTexture,
                                    const FgEgt *egt,
                                    const std::string& texture,
                                    const std::string& npcName,
                                    const std::vector<float>& raceSymCoeff,
                                    const std::vector<float>& npcSymCoeff) const
    {
        // try to regtrieve previously created morph texture
        morphTexture = Ogre::TextureManager::getSingleton().getByName(
               npcName + "_" + texture,
               Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        if (!morphTexture)
        {
            // create a blank one
            morphTexture = Ogre::TextureManager::getSingleton().createManual(
                npcName + "_" + texture, // name
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D,  // type
                egt->numRows(), egt->numColumns(), // width & height
                0,                  // number of mipmaps; FIXME: should be 2? or 1?
                Ogre::PF_BYTE_RGBA,
                Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                    // textures updated very often (e.g. each frame)
        }

        // we need the base texture
        Ogre::ResourcePtr baseTexture = Ogre::TextureManager::getSingleton().createOrRetrieve(
               texture, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first;
        if (!baseTexture)
            return false; // FIXME: throw?

        // dest: usually 256x256 (egt.numRows()*egt.numColumns())
        Ogre::HardwarePixelBufferSharedPtr pixelBuffer = morphTexture->getBuffer();
        pixelBuffer->unlock(); // prepare for blit()
        // src: can be 128x128
        Ogre::HardwarePixelBufferSharedPtr pixelBufferSrc
            = Ogre::static_pointer_cast<Ogre::Texture>(baseTexture)->getBuffer();
        pixelBufferSrc->unlock(); // prepare for blit()
        // if source and destination dimensions don't match, scaling is done
        pixelBuffer->blit(pixelBufferSrc);

        pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
        const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();
        uint8_t *pDest = static_cast<uint8_t*>(pixelBox.data);

        Ogre::Vector3 sym;
        const std::vector<Ogre::Vector3>& symTextureModes = egt->symTextureModes();
        std::size_t numSymTextureModes = egt->numSymTextureModes();

        // update the pixels with SCM and detail texture
        // NOTE: mSymTextureModes is assumed to have the image pixels in row-major order
        for (size_t i = 0; i < egt->numRows()*egt->numColumns(); ++i) // height*width, should be 256*256
        {
            // FIXME: for some reason adding the race coefficients makes it look worse
            //        even though it is clear that for shapes they are needed
            // sum all the symmetric texture modes for a given pixel i
            sym = Ogre::Vector3::ZERO;
            for (size_t j = 0; j < numSymTextureModes; ++j)
                sym += (raceSymCoeff[j] + npcSymCoeff[j]) * symTextureModes[numSymTextureModes * i + j];

            *(pDest+0) = std::min(int(*(pDest+0)+sym.x), 255);
            *(pDest+1) = std::min(int(*(pDest+1)+sym.y), 255);
            *(pDest+2) = std::min(int(*(pDest+2)+sym.z), 255);
            pDest += 4;
        }

        // Unlock the pixel buffer
        pixelBuffer->unlock();

        return true;
    }
}
