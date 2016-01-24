#include "effect.hpp"

#include "controlled.hpp"

void Nif::NiLight::SLight::read(NIFStream *nif)
{
    dimmer = nif->getFloat();
    ambient = nif->getVector3();
    diffuse = nif->getVector3();
    specular = nif->getVector3();
}

void Nif::NiLight::read(NIFStream *nif)
{
    Effect::read(nif);

    nif->getInt(); // 1
    nif->getInt(); // 1?
    light.read(nif);
}

void Nif::NiTextureEffect::read(NIFStream *nif)
{
    Effect::read(nif);

    int tmp = nif->getInt();
    if(tmp) nif->getInt(); // always 1?

    /*
       3 x Vector4 = [1,0,0,0]
       int = 2
       int = 0 or 3
       int = 2
       int = 2
    */
    nif->skip(16*4);

    texture.read(nif);

    /*
       byte = 0
       vector4 = [1,0,0,0]
       short = 0
       short = -75
       short = 0
    */
    nif->skip(23);
}

void Nif::NiTextureEffect::post(NIFFile *nif)
{
    Effect::post(nif);
    texture.post(nif);
}
