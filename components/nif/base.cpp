#include "base.hpp"

#include "extra.hpp"

void Nif::Extra::post(NIFFile *nif)
{
    if (nifVer <= 0x04020200) // up to 4.2.2.0
        extra.post(nif);

    if (nifVer >= 0x0a000100) // from 10.0.1.0
        extras.post(nif);
}

