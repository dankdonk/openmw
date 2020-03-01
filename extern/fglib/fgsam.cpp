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
//#include <cmath> // floor

//#include <OgreResourceGroupManager.h> // FIXME: for debugging only
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

        return true;
    }

    // derive the age of the NPC
    float FgSam::getAge(const std::vector<float>& raceSymCoeff, const std::vector<float>& npcSymCoeff) const
    {
        FgFile<FgCtl> ctlFile;
        const FgCtl *ctl = ctlFile.getOrLoadByName("facegen\\si.ctl");
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
    // TODO: choose the closest one?   i.e. if age is 56 choose 60
    std::string FgSam::getHeadHumanDetailTexture(float age, bool isFemale) const
    {
        // TODO: should scan the Textures\Characters\Imperial directory and get the list of
        //       detail textures rather than hard coding them here
//      std::string texture = "textures\\characters\\imperial\\";
//      if (age >= 60)
//          texture += (isFemale ? "headhumanf60.dds" : "headhumanm60.dds");
//      else if (age >= 50)
//          texture += (isFemale ? "headhumanf50.dds" : "headhumanm50.dds");
//      else if (age >= 40)
//          texture += (isFemale ? "headhumanf40.dds" : "headhumanm40.dds");
//      else if (age >= 30)
//          texture += (isFemale ? "headhumanf30.dds" : "headhumanm30.dds");
//      else if (age >= 20)
//          texture += (isFemale ? "headhumanf20.dds" : "headhumanm20.dds");
//      else if (age >= 10)
//          texture += (isFemale ? "headhumanf10.dds" : "headhumanm10.dds");
//      else
//          throw std::runtime_error("SAM: NPC age less than 10");

        if (age < 10)
            throw std::runtime_error("SAM: NPC age less than 10");

        int val = int(age) / 10;
        std::string texture = "textures\\characters\\imperial\\headhuman";
        texture += (isFemale ? "f" : "m") + std::to_string(val) + "0.dds";

        return texture;
    }

    // FIXME: deprecated
    bool FgSam::getMorphedVertices(std::vector<Ogre::Vector3> *fgMorphVertices,
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
//#if 0
        FgFile<FgCtl> ctlFile;
        const FgCtl *ctl = ctlFile.getOrLoadByName("facegen\\si.ctl");
        //FgLib::FgCtl ctl("facegen\\si.ctl", "General");
        //ctl.loadImpl();
        const std::vector<std::pair<std::vector<float>, float> >& allPrtOffsetLinearControls
            = ctl->allPrtOffsetCtl();
//#endif

        // FIXME: Not sure what to do with "Stat Morph Vertices". (see TRI in "fileformats")
        //        For headhuman.nif, the number of vertices are 0x4FB, which equals tri.numVertices(),
        //        and the number of morph vertices are 0x1CD.
        std::size_t numVertices = tri->numVertices();
        std::size_t numMorphVertices = tri->numMorphVertices();

        // FIXME: maybe just return false rather than throw?
        if (egm->numVertices() != (numVertices + numMorphVertices))
            throw std::runtime_error("SAM: Number of EGM vertices does not match that of TRI");

        fgMorphVertices->resize(numVertices);

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
            (*fgMorphVertices)[i].x = vertices[index + 0] + xMorph;
            (*fgMorphVertices)[i].y = vertices[index + 1] + yMorph;
            (*fgMorphVertices)[i].z = vertices[index + 2] + zMorph;
//#if 0
            std::string texture = getHeadHumanDetailTexture(65, false);
            size_t pos = texture.find_last_of(".");
            if (pos == std::string::npos)
                return false;

            std::string normalTexture = texture.substr(0, pos) + "_n.dds";

            if (i == 0)
            {
                xMorph = yMorph = zMorph = 0.f;
                for (std::size_t j = 0; j < 50; ++j) // for TES4 always 50 sym modes
                {
                    // NOTE: based on experiments, raceSymCoeff are needed, adding gender to scale doesn't work
                    coeff = raceSymCoeff[j] + npcSymCoeff[j];
                    scale = allPrtOffsetLinearControls[0/*age?*/].first[j]/* * symMorphModeScales[j]*/;

                    // source has all the vertices for a given mode in a group
                    //index = 3 * ((numVertices + numMorphVertices) * j + i);

                    xMorph += coeff * scale;// *symMorphModes[index + 0];
                    yMorph += coeff * scale;// *symMorphModes[index + 1];
                    zMorph += coeff * scale;// *symMorphModes[index + 2];
                }
                // doesn't always work, not sure why, unless using the abs value
                std::cout << "morph "
                    << xMorph << "," << yMorph << "," << zMorph << "," << std::dec << int(xMorph+allPrtOffsetLinearControls[0].second) << std::endl;
                    //<< vertices[index+0] << "," << vertices[index+1] << "," << vertices[index+2] << std::endl;
                    //
                    //morph 0.010403,-0.0379931,0.0316356
                    //morph 0.000575177,0.0392005,0.0331698

                    // good results but doesn't always work (e.g. ICMarketDistrict)
// morph 16.2357,16.2357,16.2357,45.9508
// CGMythicDawnAssassin04 detail 00017619 (45.9500)
// MythicDawnRobe U 4502003c
// morph 16.2357,16.2357,16.2357,45.9508
//
// morph 16.2148,16.2148,16.2148,45.9299
// CGMythicDawnAssassin03 detail 00017617 (45.9200)
// MythicDawnRobe U 4502003c
// morph 16.2148,16.2148,16.2148,45.9299
//
// morph 16.2049,16.2049,16.2049,45.92
// CGMythicDawnAssassin01 detail 00014A27 (45.9200)
// morph 16.2049,16.2049,16.2049,45.92
// MythicDawnRobe U 4502003c
//
// morph 16.1949,16.1949,16.1949,45.91
// CGMythicDawnAssassin02 detail 00014A28 (45.9100)
// morph 16.1949,16.1949,16.1949,45.91
// MythicDawnRobe U 4502003c
//
// morph 34.2849,34.2849,34.2849,64 (64.0000 note esp)
// morph 34.2849,34.2849,34.2849,64
// UrielSeptim detail 00023F2E
// MQUpperEmperorRobe U c
//
// morph 20.8849,20.8849,20.8849,50.6
// Baurus detail 00023F2A (50.6000)
//
// morph 3.47935,3.47935,3.47935,33.1945
// Glenroy detail 00023F2B (33.1900)
//
// morph -6.2751,-6.2751,-6.2751,23.44
// Renote detail 0002349F (23.44)
//
// morph 9.83984,9.83984,9.83984,39.5549
// morph 9.83984,9.83984,9.83984,39.5549
// ValenDreth detail 00025200 (39.5500)
// LowerShirt05 U ff000004
// LowerPants05 L ff000008
//
// morph 37.0305,37.0305,37.0305,66.7456
// morph 37.0305,37.0305,37.0305,66.7456
// BeggarICMarketSimplicia detail 0001C458
// LowerPants04 L 8
// LowerShirt06 U 4
// morph -14.7151,-14.7151,-14.7151,15
// ICMarketGuardPostNight03 detail 0001C34C
// 3e000004
// morph 18.701,18.701,18.701,48.4161
// ICMarketGuardPostDay03 detail 0001C34A
// 3e000004
// morph -3.2554,-3.2554,-3.2554,26.4597
// ICMarketGuardPostNight02 detail 0001C34B (26.4500)
// 3e000004
// morph -13.5251,-13.5251,-13.5251,16.19
// ICMarketGuardPostNight01 detail 0001C348
// 3e000004
// morph 9.40591,9.40591,9.40591,39.121
// ICMarketGuardPostDay02 detail 0001C349 (39.1200)
// 3e000004
// morph 33.2447,33.2447,33.2447,62.9598
// ICMarketGuardPostDay01 detail 0001C347 (62.9500)
// 3e000004
// morph -14.7151,-14.7151,-14.7151,15
// ICMarketGuardPostNight04 detail 0001C343 (15.0000)
// 3e000004
// morph 17.6107,17.6107,17.6107,47.3258
// ICMarketGuardPostDay04 detail 0001C336
// 3e000004
// morph 2.37374,2.37374,2.37374,32.0888
// ICMarketGuardPatrolNight01 detail 0001C32B (32.1600)
// 3e000004
// morph 1.69775,1.69775,1.69775,31.4129
// ICMarketGuardPostNight05 detail 0001C32A
// 3e000004
// morph -12.1751,-12.1751,-12.1751,17.54
// ICMarketGuardPatrolDay01 detail 0001C321
// 3e000004
// morph 15.1152,15.1152,15.1152,44.8303
// ICMarketGuardPostDay05 detail 0001C31F
// 3e000004

// Gender - doesn't work
//morph -0.272693,-0.272693,-0.272693,0.077086
//CGMythicDawnAssassin04 detail 00017619
//MythicDawnRobe U 4502003c
//morph -0.272693,-0.272693,-0.272693,0.077086
//
//morph -0.264488,-0.264488,-0.264488,0.0688808
//CGMythicDawnAssassin03 detail 00017617
//MythicDawnRobe U 4502003c
//morph -0.264488,-0.264488,-0.264488,0.0688808
//
//morph -0.274392,-0.274392,-0.274392,0.078785
//CGMythicDawnAssassin01 detail 00014A27
//morph -0.274392,-0.274392,-0.274392,0.078785
//MythicDawnRobe U 4502003c
//
//morph -0.264393,-0.264393,-0.264393,0.0687851
//CGMythicDawnAssassin02 detail 00014A28
//morph -0.264393,-0.264393,-0.264393,0.0687851
//MythicDawnRobe U 4502003c
//
//morph -0.714393,-0.714393,-0.714393,0.518785
//morph -0.714393,-0.714393,-0.714393,0.518785
//UrielSeptim detail 00023F2E
//MQUpperEmperorRobe U c
//
//morph -2.76439,-2.76439,-2.76439,2.56878
//Baurus detail 00023F2A
//morph -2.18337,-2.18337,-2.18337,1.98776
//Glenroy detail 00023F2B
//morph 2.28561,2.28561,2.28561,2.09
//Renote detail 0002349F
//morph -2.13382,-2.13382,-2.13382,1.93822
//morph -2.13382,-2.13382,-2.13382,1.93822
//ValenDreth detail 00025200
//LowerShirt05 U ff000004
//LowerPants05 L ff000008
            }
            // vert 0.616405,9.27905,5.92473
            // 0.616405*x + 9.27905*y + 5.92473*z + 29.715103 = 0.135416
            // can't solve for x, y, z :-(
//#endif
        }
//#if 0
        // start experiment - age
        //
        //     aⱼ = aⱼ. p + oⱼ
        //
        //
        // 0.135416  0.281922
        // 65        64
        //
        // -1.441575 1st age coeff
        // o = 29.715103
        //
        //
        //     p' = p + Δaⱼ
        //
        //     Δ = (aⱼ'- aⱼ) / (|aⱼ|²)
        //
        // p  = tri vertex
        // p' = morphed vertex
        //
        //
        //     aᵢ. aⱼ = ?
        //
        //     aⱼ'- aⱼ = ?
        //
        //     p' = p + ∑(Δᵢ. aᵢ)
        //     p' - p =  ∑(Δᵢ. aᵢ) /* e.g. fgVertices[i] - vertices[i] = morph[i] */
        //
        // end experiment
//#endif
        return true;
    }

    bool FgSam::getMorphedTexture(Ogre::TexturePtr fgTexture,
                                    const std::string& mesh,
                                    const std::string& npcName, // FIXME: for debugging only
                                    const std::vector<float>& raceSymCoeff,
                                    const std::vector<float>& raceAsymCoeff,
                                    const std::vector<float>& npcSymCoeff,
                                    const std::vector<float>& npcAsymCoeff) const
    {
        FgFile<FgEgt> egtFile;
        const FgEgt *egt = egtFile.getOrLoadByName(mesh);

        if (egt == nullptr)
            return false; // texture morph not possible

        return getMorphedTexture(fgTexture, egt, npcName, raceSymCoeff, raceAsymCoeff, npcSymCoeff, npcAsymCoeff);
    }

    bool FgSam::getMorphedTexture(Ogre::TexturePtr fgTexture,
                                    const FgEgt *egt,
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
        return false;
    }
}
