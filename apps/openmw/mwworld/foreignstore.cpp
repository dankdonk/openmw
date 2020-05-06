#include "foreignstore.hpp"
#include "esmstore.hpp"

#include <components/esm/esmreader.hpp>
#include <components/esm/esm4reader.hpp>

#include <components/loadinglistener/loadinglistener.hpp>

#include <extern/esm4/pgrd.hpp> // this one is not in esmstore
#include <extern/esm4/formid.hpp>

#include <stdexcept>
#include <sstream>

//#define DEBUG_FORMID
#undef DEBUG_FORMID

namespace MWWorld
{
    ForeignId::ForeignId(ESM4::FormId formId, bool isDeleted)
        : mId(formId), mIsDeleted(isDeleted)
    {}

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
#ifdef DEBUG_FORMID
        ESM4::FormId formId = 0;
        if (ESM4::isFormId(id))
            formId = ESM4::stringToFormId(id);
#else
        ESM4::FormId formId = ESM4::stringToFormId(id);
#endif
        return search(formId);
    }

    template<typename T>
    const T *ForeignStore<T>::search(ESM4::FormId id) const
    {
        // NOTE: below logic is lifted from MWWorld::Store; the differences are that
        // FormId is used instead of std::string and hence no need to convert to lowercase
        typename Dynamic::const_iterator dit = mDynamic.find(id);
        if (dit != mDynamic.end())
            return &dit->second;

        typename Static::const_iterator it = mStatic.find(id);
        if (it != mStatic.end())
            return &(it->second);

        return nullptr;
    }

    template<typename T>
    bool ForeignStore<T>::isDynamic(const std::string &id) const
    {
#ifdef DEBUG_FORMID
        ESM4::FormId formId = 0;
        if (ESM4::isFormId(id))
            formId = ESM4::stringToFormId(id);
#else
        ESM4::FormId formId = ESM4::stringToFormId(id);
#endif
        typename Dynamic::const_iterator dit = mDynamic.find(formId);
        return (dit != mDynamic.end());
    }

    template<typename T>
    const T *ForeignStore<T>::find(const std::string &id) const
    {
#ifdef DEBUG_FORMID
        ESM4::FormId formId = 0;
        if (ESM4::isFormId(id))
            formId = ESM4::stringToFormId(id);
#else
        ESM4::FormId formId = ESM4::stringToFormId(id);
#endif
        const T *ptr = search(formId);
        if (ptr == nullptr) {
            std::ostringstream msg;
            // FIXME: getRecordType() not implemented
            msg << /*T::getRecordType() <<*/ " '" << id << "' not found";
            throw std::runtime_error(msg.str());
        }
        return ptr;
    }

    template<typename T>
    const T *ForeignStore<T>::findRandom(const std::string &id) const
    {
        std::ostringstream msg;
        // FIXME: getRecordType() not implemented
        msg << /*T::getRecordType() <<*/ " starting with '"<<id<<"' not found";
        throw std::runtime_error(msg.str());
    }

    template<typename T>
    RecordId ForeignStore<T>::load(ESM::ESMReader& esm)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
        ForeignId result = loadForeign(reader);

        std::string id = ESM4::formIdToString(result.mId);
        return RecordId(id, result.mIsDeleted); // NOTE: id is uppercase (not that it matters)
    }

    template<typename T>
    ForeignId ForeignStore<T>::loadForeign(ESM4::Reader& reader)
    {
        T record;
        record.load(reader);

        bool isDeleted = (record.mFlags & ESM4::Rec_Deleted) != 0;

        std::pair<typename Static::iterator, bool> inserted
            = mStatic.insert(std::make_pair(record.mFormId, record));

        if (inserted.second)
            mShared.push_back(&inserted.first->second);
        else
            inserted.first->second = record;

        return ForeignId(record.mFormId, isDeleted);
    }

    template<typename T>
    void ForeignStore<T>::setUp()
    {
        // FIXME
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
        return (int) mDynamic.size();
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
    //
    // FIXME: is this useful at all?
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
        std::pair<typename Dynamic::iterator, bool> result =
            mDynamic.insert(std::pair<ESM4::FormId, T>(item.mFormId, item));

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
        std::pair<typename Static::iterator, bool> result =
            mStatic.insert(std::pair<ESM4::FormId, T>(item.mFormId, item));

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
#ifdef DEBUG_FORMID
        ESM4::FormId formId = 0;
        if (ESM4::isFormId(id))
            formId = ESM4::stringToFormId(id);
#else
        ESM4::FormId formId = ESM4::stringToFormId(id);
#endif

        typename std::map<ESM4::FormId, T>::iterator it = mStatic.find(formId);

        if (it != mStatic.end()) {
            // delete from the static part of mShared
            typename std::vector<T *>::iterator sharedIter = mShared.begin();
            typename std::vector<T *>::iterator end = sharedIter + mStatic.size();

            while (sharedIter != mShared.end() && sharedIter != end) {
                if((*sharedIter)->mFormId == formId) {
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
    bool ForeignStore<T>::erase(const std::string &id) // FIXME: is this ever used?
    {
#ifdef DEBUG_FORMID
        ESM4::FormId formId = 0;
        if (ESM4::isFormId(id))
            formId = ESM4::stringToFormId(id);
#else
        ESM4::FormId formId = ESM4::stringToFormId(id);
#endif
        return erase(formId);
    }

    template<typename T>
    bool ForeignStore<T>::erase(ESM4::FormId formId)
    {
        typename Dynamic::iterator it = mDynamic.find(formId);
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
        return erase(item.mFormId);
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

        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
        record.load(reader);

        bool isDeleted = (record.mFlags & ESM4::Rec_Deleted) != 0;

        insert(record);

        std::string id = ESM4::formIdToString(record.mFormId);
        return RecordId(id, isDeleted);
    }
}

template class MWWorld::ForeignStore<ESM4::Hair>;
template class MWWorld::ForeignStore<ESM4::Eyes>;
template class MWWorld::ForeignStore<ESM4::Race>;
template class MWWorld::ForeignStore<ESM4::ActorCharacter>;
template class MWWorld::ForeignStore<ESM4::ActorCreature>;
template class MWWorld::ForeignStore<ESM4::LandTexture>;
template class MWWorld::ForeignStore<ESM4::Script>;
template class MWWorld::ForeignStore<ESM4::Dialogue>;
template class MWWorld::ForeignStore<ESM4::DialogInfo>;
template class MWWorld::ForeignStore<ESM4::Quest>;
template class MWWorld::ForeignStore<ESM4::AIPackage>;
template class MWWorld::ForeignStore<ESM4::Pathgrid>;
template class MWWorld::ForeignStore<ESM4::BodyPart>;
template class MWWorld::ForeignStore<ESM4::HeadPart>;
template class MWWorld::ForeignStore<ESM4::LightingTemplate>;
template class MWWorld::ForeignStore<ESM4::Music>;
template class MWWorld::ForeignStore<ESM4::MediaLocationController>;
template class MWWorld::ForeignStore<ESM4::MediaSet>;
template class MWWorld::ForeignStore<ESM4::DefaultObj>;
template class MWWorld::ForeignStore<ESM4::Region>;
template class MWWorld::ForeignStore<ESM4::PlacedGrenade>;
// Foreign Referenceables
template class MWWorld::ForeignStore<ESM4::Sound>;
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
template class MWWorld::ForeignStore<ESM4::LevelledCreature>;
template class MWWorld::ForeignStore<ESM4::SoulGem>;
template class MWWorld::ForeignStore<ESM4::Key>;
template class MWWorld::ForeignStore<ESM4::Potion>;
template class MWWorld::ForeignStore<ESM4::Subspace>;
template class MWWorld::ForeignStore<ESM4::SigilStone>;
template class MWWorld::ForeignStore<ESM4::LevelledItem>;
template class MWWorld::ForeignStore<ESM4::LevelledNpc>;
template class MWWorld::ForeignStore<ESM4::IdleMarker>;
template class MWWorld::ForeignStore<ESM4::MovableStatic>;
template class MWWorld::ForeignStore<ESM4::TextureSet>;
template class MWWorld::ForeignStore<ESM4::Scroll>;
template class MWWorld::ForeignStore<ESM4::ArmorAddon>;
template class MWWorld::ForeignStore<ESM4::Terminal>;
template class MWWorld::ForeignStore<ESM4::TalkingActivator>;
template class MWWorld::ForeignStore<ESM4::Note>;
template class MWWorld::ForeignStore<ESM4::AcousticSpace>;
template class MWWorld::ForeignStore<ESM4::ItemMod>;
template class MWWorld::ForeignStore<ESM4::PlaceableWater>;
template class MWWorld::ForeignStore<ESM4::StaticCollection>;
template class MWWorld::ForeignStore<ESM4::AnimObject>;
