#include "foreignstore.hpp"
#include "esmstore.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/esm4reader.hpp>

#include <components/loadinglistener/loadinglistener.hpp>

#include <extern/esm4/pgrd.hpp> // this one is not in esmstore
#include <extern/esm4/formid.hpp>

#include <components/misc/rng.hpp>
#include <stdexcept>
#include <sstream>

namespace MWWorld
{
    template<typename T>
    ForeignStore<T>::ForeignStore()
    {
    }

    template<typename T>
    ForeignStore<T>::ForeignStore(const ForeignStore<T>& orig)
        : mStatic(orig.mStatic)
    {
    }

    template<typename T>
    void ForeignStore<T>::clearDynamic()
    {
        // remove the dynamic part of mShared
        assert(mShared.size() >= mStatic.size());
        mShared.erase(mShared.begin() + mStatic.size(), mShared.end());
        mDynamic.clear();
    }

    template<typename T>
    const T *ForeignStore<T>::search(const std::string &id) const
    {
#if 0
        T item;
        item.mEditorId = Misc::StringUtils::lowerCase(id);

        typename Dynamic::const_iterator dit = mDynamic.find(item.mEditorId);
        if (dit != mDynamic.end()) {
            return &dit->second;
        }

        typename std::map<std::string, T>::const_iterator it = mStatic.find(item.mEditorId);

        if (it != mStatic.end() && Misc::StringUtils::ciEqual(it->second.mEditorId, id)) {
            return &(it->second);
        }
#else
        // FIXME: just loop through for now, will need to maintain two maps
        typename Dynamic::const_iterator dit = mDynamic.begin();
        for (; dit != mDynamic.end(); ++dit)
        {
            if (dit->second.mEditorId == id)
                return &dit->second;
        }

        typename Static::const_iterator it = mStatic.begin();
        for (; it != mStatic.end(); ++it)
        {
            if (it->second.mEditorId == id)
                return &it->second;
        }
#endif
        return 0;
    }

    template<typename T>
    const T *ForeignStore<T>::search(ESM4::FormId id) const
    {
#if 1
        T item;
        // FIXME: just loop through for now, will need to maintain two maps
        typename Dynamic::const_iterator dit = mDynamic.begin();
        for (; dit != mDynamic.end(); ++dit)
        {
            if (dit->second.mFormId == id)
                return &dit->second;
        }

        typename Static::const_iterator it = mStatic.begin();
        for (; it != mStatic.end(); ++it)
        {
            if (it->second.mFormId == id)
                return &it->second;
        }
#else
        typename std::map<ESM4::FormId, T>::const_iterator dit = mDynamic.find(id);
        if (dit != mDynamic.end())
            return &dit->second;

        typename std::map<ESM4::FormId, T>::const_iterator it = mStatic.find(id);

        if (it != mStatic.end())
            return &(it->second);
#endif
        return 0;
    }

    template<typename T>
    bool ForeignStore<T>::isDynamic(const std::string &id) const
    {
        typename Dynamic::const_iterator dit = mDynamic.find(id);
        return (dit != mDynamic.end());
    }
    template<typename T>
    const T *ForeignStore<T>::searchRandom(const std::string &id) const
    {
        // FIXME
#if 0
        std::vector<const T*> results;
        std::for_each(mShared.begin(), mShared.end(), GetForeignRecords<T>(id, &results));
        if(!results.empty())
            return results[Misc::Rng::rollDice(results.size())];
#endif
        return NULL;
    }
    template<typename T>
    const T *ForeignStore<T>::find(const std::string &id) const
    {
        const T *ptr = search(id);
        if (ptr == 0) {
            std::ostringstream msg;
            // FIXME
            //msg << T::getRecordType() << " '" << id << "' not found";
            throw std::runtime_error(msg.str());
        }
        return ptr;
    }
    template<typename T>
    const T *ForeignStore<T>::findRandom(const std::string &id) const
    {
        const T *ptr = searchRandom(id);
        if(ptr == 0)
        {
            std::ostringstream msg;
            // FIXME
            //msg << T::getRecordType() << " starting with '"<<id<<"' not found";
            throw std::runtime_error(msg.str());
        }
        return ptr;
    }
    template<typename T>
    RecordId ForeignStore<T>::load(ESM::ESMReader &esm)
    {
        T record;
        bool isDeleted = false;
        std::string id;

        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

        record.load(reader);

        ESM4::formIdToString(record.mFormId, id);
        isDeleted = (record.mFlags & ESM4::Rec_Deleted) != 0;

        Misc::StringUtils::lowerCaseInPlace(id);

        std::pair<typename Static::iterator, bool> inserted = mStatic.insert(std::make_pair(id, record));
        if (inserted.second)
            mShared.push_back(&inserted.first->second);
        else
            inserted.first->second = record;

        return RecordId(id, isDeleted);
    }
    template<typename T>
    void ForeignStore<T>::setUp()
    {
    }

    template<typename T>
    typename ForeignStore<T>::iterator ForeignStore<T>::begin() const
    {
        return mShared.begin();
    }
    template<typename T>
    typename ForeignStore<T>::iterator ForeignStore<T>::end() const
    {
        return mShared.end();
    }

    template<typename T>
    size_t ForeignStore<T>::getSize() const
    {
        return mShared.size();
    }

    template<typename T>
    int ForeignStore<T>::getDynamicSize() const
    {
        return (int) mDynamic.size(); // FIXME: consider making it return size_t
    }
    template<typename T>
    void ForeignStore<T>::listForeignIdentifier(std::vector<ESM4::FormId> &list) const
    {
        list.reserve(list.size() + getSize());
        typename std::vector<T *>::const_iterator it = mShared.begin();
        for (; it != mShared.end(); ++it) {
            list.push_back((*it)->mFormId);
        }
    }
    // Used by ESMStore::setUp() to map references to stores (of referenceable object types)
    // Basically pulls all the EditorId strings out of the records and puts them in the list.
    template<typename T>
    void ForeignStore<T>::listIdentifier(std::vector<std::string> &list) const
    {
        list.reserve(list.size() + getSize());
        typename std::vector<T *>::const_iterator it = mShared.begin();
        for (; it != mShared.end(); ++it) {
            list.push_back((*it)->mEditorId);
        }
    }
    template<typename T>
    T *ForeignStore<T>::insert(const T &item)
    {
        std::string id = Misc::StringUtils::lowerCase(item.mEditorId);
        std::pair<typename Dynamic::iterator, bool> result =
            mDynamic.insert(std::pair<std::string, T>(id, item));
        T *ptr = &result.first->second;
        if (result.second) {
            mShared.push_back(ptr);
        } else {
            *ptr = item;
        }
        return ptr;
    }
    template<typename T>
    T *ForeignStore<T>::insertStatic(const T &item)
    {
        std::string id = Misc::StringUtils::lowerCase(item.mEditorId);
        std::pair<typename Static::iterator, bool> result =
            mStatic.insert(std::pair<std::string, T>(id, item));
        T *ptr = &result.first->second;
        if (result.second) {
            mShared.push_back(ptr);
        } else {
            *ptr = item;
        }
        return ptr;
    }
    template<typename T>
    bool ForeignStore<T>::eraseStatic(const std::string &id)
    {
        T item;
        item.mEditorId = Misc::StringUtils::lowerCase(id);

        typename std::map<std::string, T>::iterator it = mStatic.find(item.mEditorId);

        if (it != mStatic.end() && Misc::StringUtils::ciEqual(it->second.mEditorId, id)) {
            // delete from the static part of mShared
            typename std::vector<T *>::iterator sharedIter = mShared.begin();
            typename std::vector<T *>::iterator end = sharedIter + mStatic.size();

            while (sharedIter != mShared.end() && sharedIter != end) {
                if((*sharedIter)->mEditorId == item.mEditorId) {
                    mShared.erase(sharedIter);
                    break;
                }
                ++sharedIter;
            }
            mStatic.erase(it);
        }
        return true;
    }

    template<typename T>
    bool ForeignStore<T>::erase(const std::string &id)
    {
        std::string key = Misc::StringUtils::lowerCase(id);
        typename Dynamic::iterator it = mDynamic.find(key);
        if (it == mDynamic.end()) {
            return false;
        }
        mDynamic.erase(it);

        // have to reinit the whole shared part
        assert(mShared.size() >= mStatic.size());
        mShared.erase(mShared.begin() + mStatic.size(), mShared.end());
        for (it = mDynamic.begin(); it != mDynamic.end(); ++it) {
            mShared.push_back(&it->second);
        }
        return true;
    }

    template<typename T>
    bool ForeignStore<T>::erase(const T &item)
    {
        return erase(item.mEditorId);
    }

    template<typename T>
    void ForeignStore<T>::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        for (typename Dynamic::const_iterator iter (mDynamic.begin()); iter!=mDynamic.end();
             ++iter)
        {
            // FIXME
#if 0
            writer.startRecord (T::sRecordId);
            iter->second.save (writer);
            writer.endRecord (T::sRecordId);
#endif
        }
    }

    template<typename T>
    RecordId ForeignStore<T>::read(ESM::ESMReader& esm)
    {
        T record;
        bool isDeleted = false;
        std::string id;

        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
        record.load(reader);

        ESM4::formIdToString(record.mFormId, id);
        Misc::StringUtils::lowerCaseInPlace(id); // not sure if case folding is needed here
        isDeleted = (record.mFlags & ESM4::Rec_Deleted) != 0;

        return RecordId(id, isDeleted);
    }
}

template class MWWorld::ForeignStore<ESM4::Hair>;
template class MWWorld::ForeignStore<ESM4::Eyes>;
template class MWWorld::ForeignStore<ESM4::Race>;
template class MWWorld::ForeignStore<ESM4::ActorCharacter>;
template class MWWorld::ForeignStore<ESM4::ActorCreature>;
template class MWWorld::ForeignStore<ESM4::Sound>;
template class MWWorld::ForeignStore<ESM4::LandTexture>;
template class MWWorld::ForeignStore<ESM4::Script>;
template class MWWorld::ForeignStore<ESM4::Dialog>;
template class MWWorld::ForeignStore<ESM4::DialogInfo>;
template class MWWorld::ForeignStore<ESM4::Quest>;
template class MWWorld::ForeignStore<ESM4::AIPackage>;
template class MWWorld::ForeignStore<ESM4::Pathgrid>;
// Foreign Referenceables
template class MWWorld::ForeignStore<ESM4::Activator>;
template class MWWorld::ForeignStore<ESM4::Apparatus>;
template class MWWorld::ForeignStore<ESM4::Armor>;
template class MWWorld::ForeignStore<ESM4::Book>;
template class MWWorld::ForeignStore<ESM4::Clothing>;
template class MWWorld::ForeignStore<ESM4::Container>;
template class MWWorld::ForeignStore<ESM4::Door>;
template class MWWorld::ForeignStore<ESM4::Ingredient>;
template class MWWorld::ForeignStore<ESM4::Light>;
template class MWWorld::ForeignStore<ESM4::MiscItem>;
template class MWWorld::ForeignStore<ESM4::Static>;
template class MWWorld::ForeignStore<ESM4::Grass>;
template class MWWorld::ForeignStore<ESM4::Tree>;
template class MWWorld::ForeignStore<ESM4::Flora>;
template class MWWorld::ForeignStore<ESM4::Furniture>;
template class MWWorld::ForeignStore<ESM4::Weapon>;
template class MWWorld::ForeignStore<ESM4::Ammo>;
template class MWWorld::ForeignStore<ESM4::Npc>;
template class MWWorld::ForeignStore<ESM4::Creature>;
template class MWWorld::ForeignStore<ESM4::LeveledCreature>;
template class MWWorld::ForeignStore<ESM4::SoulGem>;
template class MWWorld::ForeignStore<ESM4::Key>;
template class MWWorld::ForeignStore<ESM4::Potion>;
template class MWWorld::ForeignStore<ESM4::Subspace>;
template class MWWorld::ForeignStore<ESM4::SigilStone>;
template class MWWorld::ForeignStore<ESM4::LeveledItem>;
template class MWWorld::ForeignStore<ESM4::LeveledActor>;
template class MWWorld::ForeignStore<ESM4::IdleMarker>;
template class MWWorld::ForeignStore<ESM4::MovableStatic>;
template class MWWorld::ForeignStore<ESM4::TextureSet>;
template class MWWorld::ForeignStore<ESM4::Scroll>;
template class MWWorld::ForeignStore<ESM4::ArmorAddon>;
template class MWWorld::ForeignStore<ESM4::HeadPart>;
template class MWWorld::ForeignStore<ESM4::Terminal>;
template class MWWorld::ForeignStore<ESM4::TalkingActivator>;
template class MWWorld::ForeignStore<ESM4::Note>;
template class MWWorld::ForeignStore<ESM4::BodyPart>;
//
template class MWWorld::ForeignStore<ESM4::AnimObject>;
