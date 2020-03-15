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

*/
#ifndef FGLIB_FGSAM_H
#define FGLIB_FGSAM_H

#include <string>
#include <vector>

#include <OgreTexture.h>

namespace NiBtOgre
{
    class NiModel;
}

namespace FgLib
{
    class FgEgt;

    class FgSam
    {
    public:
        FgSam() {}
        ~FgSam() {}

        // returns the age of the NPC based on the NPC and Race FGGS coefficients
        float getAge(const std::vector<float>& raceSymCoeff, const std::vector<float>& npcSymCoeff) const;

        // returns the age based detail modulation texture for headhuman.dds
        std::string getHeadHumanDetailTexture(const std::string& mesh, float age, bool isFemale) const;

        // returns the npc specific face detail texture
        std::string getNpcDetailTexture_0(const std::string& npcFormIdString) const;

        // returns true if TRI and EGM files are found (or loaded) to build the morphed vertices
        //
        // NOTE: some NIF don't have associated TRI file; e.g. emperor.nif and emperor.egm
        // exists but no emperor.tri in such cases the vertices from the NIF are used
        // (which are a little different to the vertices in the TRI file but they'll have to do)
        bool buildMorphedVertices(std::vector<Ogre::Vector3>& fgMorphVertices,
                                  const std::vector<Ogre::Vector3>& fgVertices,
                                  const std::string& nif,
                                  const std::vector<float>& raceSymCoeff,
                                  const std::vector<float>& raceAsymCoeff,
                                  const std::vector<float>& npcSymCoeff,
                                  const std::vector<float>& npcAsymCoeff) const;

        // populates fgTexture
        // returns true if EGT file is either found in the map or loaded from disk *and*
        //              fgTexture is in the correct format & dimensions
        //
        // fgTexture must be in Ogre::PF_BYTE_RGBA format
        // fgTexture must be type Ogre::TEX_TYPE_2D
        // fgTexture dimensions must match the corresponding EGT file
        bool getMorphedTexture(Ogre::TexturePtr& morphTexture,
                               const std::string& mesh,
                               const std::string& texture,
                               const std::string& npcName,
                               const std::vector<float>& raceSymCoeff,
                               const std::vector<float>& npcSymCoeff) const;

        // convineence method for human bodies only
        bool getMorphedBodyTexture(Ogre::TexturePtr& morphTexture,
                               const std::string& mesh,
                               const std::string& texture,
                               const std::string& npcName,
                               const std::vector<float>& raceSymCoeff,
                               const std::vector<float>& npcSymCoeff) const;

    private:
        bool getMorphedTexture(Ogre::TexturePtr& morphTexture,
                               const FgEgt *egt,
                               const std::string& texture,
                               const std::string& npcName,
                               const std::vector<float>& raceSymCoeff,
                               const std::vector<float>& npcSymCoeff) const;
    };
}

#endif // FGLIB_FGSAM_H
