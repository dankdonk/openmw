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
#ifndef NIBTOGRE_FGSAM_H
#define NIBTOGRE_FGSAM_H

#include <map>
#include <string>
#include <vector>
#include <memory>

#include <OgreTexture.h>

#include "fgtri.hpp"
#include "fgegm.hpp"
#include "fgegt.hpp"

namespace NiBtOgre
{
    class FgSam
    {
    public:
        struct FgFiles
        {
            FgTri tri;
            FgEgm egm;
            FgEgt egt; // NOTE: some, e.g. eyes don't have EGT

            FgFiles(const std::string& base);
            ~FgFiles () {}

        private:
            FgFiles();
        };

    private:
        // NOTE: There is no real management of FaceGen resources since there
        //       aren't that many of them (i.e. they are never unloaded).
        // NOTE: No fancy hashing of the std::string keys, either.
        static std::map<std::string, std::unique_ptr<FgFiles> > sFgFilesMap;

    public:
        FgSam();
        ~FgSam();

        // populates fgVertices
        // returns true if TRI and EGM files are either found in the map or loaded from disk
        const FgFiles *getMorphedVertices(std::vector<Ogre::Vector3>& fgVertices,
                                const std::string& mesh,
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
        const FgFiles *getMorphedTextures(Ogre::TexturePtr fgTexture,
                                const std::string& mesh,
                                const std::string& npcName, // FIXME: for debugging only
                                const std::vector<float>& raceSymCoeff,
                                const std::vector<float>& raceAsymCoeff,
                                const std::vector<float>& npcSymCoeff,
                                const std::vector<float>& npcAsymCoeff) const;

        const FgFiles *getMorphedTextures(Ogre::TexturePtr fgTexture,
                                const FgFiles *fgFiles,
                                const std::string& npcName, // FIXME: for debugging only
                                const std::vector<float>& raceSymCoeff,
                                const std::vector<float>& raceAsymCoeff,
                                const std::vector<float>& npcSymCoeff,
                                const std::vector<float>& npcAsymCoeff) const;

    private:
        const FgFiles *getOrLoadByName(const std::string& mesh) const;
    };
}

#endif // NIBTOGRE_FGSAM_H
