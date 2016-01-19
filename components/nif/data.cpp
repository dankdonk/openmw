#include "data.hpp"
#include "node.hpp"

namespace Nif
{
void NiSkinInstance::post(NIFFile *nif)
{
    data.post(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        skinPartition.post(nif);
    root.post(nif);
    bones.post(nif);

    if(data.empty() || root.empty())
        nif->fail("NiSkinInstance missing root or data");

    size_t bnum = bones.length();
    if(bnum != data->bones.size())
        nif->fail("Mismatch in NiSkinData bone count");

    root->makeRootBone(&data->trafo);

    for(unsigned int i = 0; i < bnum; i++)
    {
        if(bones[i].empty())
            nif->fail("Oops: Missing bone! Don't know how to handle this.");
        bones[i]->makeBone(i, data->bones[i]);
    }
}

} // Namespace
