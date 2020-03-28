#include "esmloader.hpp"
#include "esmstore.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/esm4reader.hpp>

namespace MWWorld
{

EsmLoader::EsmLoader(MWWorld::ESMStore& store, std::vector<std::vector<ESM::ESMReader*> >& readers,
  ToUTF8::Utf8Encoder* encoder, Loading::Listener& listener)
  : ContentLoader(listener)
  , mEsm(readers)
  , mStore(store)
  , mEncoder(encoder)
{
}

// increment indices for each game version
void EsmLoader::load(const boost::filesystem::path& filepath, std::vector<std::vector<std::string> >& contentFiles)
{
    int tesVerIndex = 0; // FIXME: hard coded, 0 = MW
    int index = (int)/*compiler warning*/ contentFiles[tesVerIndex].size();

    ContentLoader::load(filepath.filename(), contentFiles); // set the label on the loading bar

    ESM::ESMReader *lEsm = new ESM::ESMReader();
    lEsm->setEncoder(mEncoder);
    lEsm->setGlobalReaderList(&mEsm[tesVerIndex]);  // global reader list is used by ESMStore::load only
    lEsm->open(filepath.string());

    int esmVer = lEsm->getVer();
    bool isTes4 = esmVer == ESM::VER_080 || esmVer == ESM::VER_100;
    bool isTes5 = esmVer == ESM::VER_094 || esmVer == ESM::VER_17;
    if (isTes4 || isTes5)
    {
        lEsm->close();
        delete lEsm;
        ESM::ESM4Reader *esm = new ESM::ESM4Reader(isTes4); // NOTE: TES4 headers are 4 bytes shorter
        esm->setEncoder(mEncoder);
        tesVerIndex = isTes4 ? 1 : 2; // FIXME: hard coded, 2 = TES5
        index = (int)/*compiler warning*/ contentFiles[tesVerIndex].size();
        contentFiles[tesVerIndex].push_back(filepath.filename().string());
        esm->setIndex(index);
        esm->reader().setModIndex(index);
        esm->openTes4File(filepath.string());
        esm->reader().updateModIndices(contentFiles[tesVerIndex]);
        // FIXME: this does not work well (copies the base class pointer)
        //i.e. have to check TES4/TES5 versions each time before use within EsmStore::load,
        //static casting as required
        mEsm[tesVerIndex].push_back(esm);
    }
    else
    {
        contentFiles[tesVerIndex].push_back(filepath.filename().string());
        lEsm->setIndex(index);
        mEsm[tesVerIndex].push_back(lEsm);
    }

    mStore.load(*mEsm[tesVerIndex][index], &mListener);
}

} /* namespace MWWorld */
