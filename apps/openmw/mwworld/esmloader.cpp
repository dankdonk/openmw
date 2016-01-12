#include "esmloader.hpp"
#include "esmstore.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/esm4reader.hpp>

namespace MWWorld
{

EsmLoader::EsmLoader(MWWorld::ESMStore& store, std::vector<ESM::ESMReader*>& readers,
  ToUTF8::Utf8Encoder* encoder, Loading::Listener& listener)
  : ContentLoader(listener)
  , mEsm(readers)
  , mStore(store)
  , mEncoder(encoder)
{
}

void EsmLoader::load(const boost::filesystem::path& filepath, int& index)
{
    ContentLoader::load(filepath.filename(), index);

    ESM::ESMReader *lEsm = new ESM::ESMReader();
    lEsm->setEncoder(mEncoder);
    lEsm->setIndex(index);
    lEsm->setGlobalReaderList(&mEsm);
    lEsm->open(filepath.string());

    bool isTes4 = lEsm->getVer() == ESM::VER_080;
    if (isTes4 ||
            lEsm->getVer() == ESM::VER_094 || lEsm->getVer() == ESM::VER_17) // TES5
    {
        delete lEsm;
        ESM::ESM4Reader *esm = new ESM::ESM4Reader(isTes4);
        esm->setEncoder(mEncoder);
        esm->setIndex(index);
        esm->openTes4File(filepath.string());
        mEsm[index] = esm; // FIXME: this does not work (copies base class)
    }
    else
        mEsm[index] = lEsm;

    mStore.load(*mEsm[index], &mListener);
}

} /* namespace MWWorld */
