#ifndef GAME_MWWORLD_CELLREFLIST_H
#define GAME_MWWORLD_CELLREFLIST_H

#include <list>
#include <vector>
#include <map>

#include "livecellref.hpp"

namespace MWWorld
{
#if 1
    struct CellRefStoreBase
    {
        virtual ~CellRefStoreBase() {}

        virtual LiveCellRefBase *find(const ESM4::FormId id) { return nullptr; }
        virtual LiveCellRefBase *find(const std::string& name) = 0;
        virtual std::vector<LiveCellRefBase*> search(std::int32_t x, std::int32_t y,
                std::size_t range = 0, std::size_t exclude = 0) { return std::vector<LiveCellRefBase*>(); }
        virtual LiveCellRefBase *searchViaHandle(const std::string& handle) = 0;
    };

    /// \brief Collection of references of one type
    template <typename X>
    struct CellRefList
    {
        typedef LiveCellRef<X> LiveRef;
        typedef std::list<LiveRef> List;
        List mList;

        typedef std::vector<LiveRef> Vect;
        Vect mVect;

        //std::map<ESM4::FormId, List::iterator> mFormIdMap;
        typedef std::map<ESM4::FormId, LiveCellRefBase*> FormIdMap;
        FormIdMap mFormIdMap;

        typedef std::map<std::pair<std::int32_t, std::int32_t>, std::vector<LiveCellRefBase*> > GridMap;
        GridMap mGridMap;

        /// Search for the given reference in the given reclist from
        /// ESMStore. Insert the reference into the list if a match is
        /// found. If not, throw an exception.
        /// Moved to cpp file, as we require a custom compare operator for it,
        /// and the build will fail with an ugly three-way cyclic header dependence
        /// so we need to pass the instantiation of the method to the linker, when
        /// all methods are known.
        void load (ESM::CellRef &ref, bool deleted, const MWWorld::ESMStore &esmStore);
        void load (ESM4::Reference &ref, bool deleted, const MWWorld::ESMStore &esmStore, bool dummy = false);
        void load (ESM4::ActorCreature &ref, bool deleted, const MWWorld::ESMStore &esmStore, bool dummy = false);
        void load (ESM4::ActorCharacter &ref, bool deleted, const MWWorld::ESMStore &esmStore, bool dummy = false);

        LiveCellRefBase *find (const std::string& name)
        {
            for (typename List::iterator iter (mList.begin()); iter!=mList.end(); ++iter)
                if (!iter->mData.isDeletedByContentFile()
                        && (iter->mRef.hasContentFile() || iter->mData.getCount() > 0)
                        && iter->mRef.getRefId() == name)
                    return &*iter;

            return 0;
        }

        // used by copyToCellImpl()
        LiveCellRefBase *insert (const LiveRef &item)
        {
            ESM4::FormId formId = item.mRef.getFormId();

            mList.push_back(item);

            LiveCellRefBase *refPtr = &mList.back();
            mFormIdMap[formId] = refPtr;

            // NOTE: assumed that it is not possible to insert records to a dummy cell,
            //       hence not updating GridMap

            return refPtr;
        }

        LiveCellRefBase *searchViaHandle (const std::string& handle)
        {
            for (typename List::iterator iter (mList.begin()); iter!=mList.end(); ++iter)
                if (iter->mData.getBaseNode() &&
                    iter->mData.getHandle()==handle)
                    return &*iter;

            return 0;
        }
    };
#else
    struct CellRefStoreBase
    {
        virtual ~CellRefStoreBase() {}

        //virtual LiveCellRefBase *find(const ESM4::FormId id) = 0;
        virtual LiveCellRefBase *find(const std::string& id) = 0;
        //virtual LiveRef *find (const std::string& name) = 0;
        //virtual std::vector<LiveCellRefBase*> search(std::int32_t x, std::int32_t y, std::size_t range = 0, std::size_t range2 = 0) = 0;
        virtual LiveCellRefBase *searchViaHandle(const std::string& handle) = 0;
        //LiveCellRef<X> *searchViaHandle (const std::string& handle) = 0;
    };

    template <typename X>
    struct CellRefList : public CellRefStoreBase
    {
        typedef LiveCellRef<X> LiveRef;
        //typedef std::list<LiveRef> List;
        typedef std::vector<LiveRef> List;
        List mList;

        typedef std::map<std::pair<std::int32_t, std::int32_t>, std::vector<std::size_t> > GridMap;
        GridMap mGridMap;

        std::map<ESM4::FormId, std::size_t> mIdMap;

        /// Search for the given reference in the given reclist from
        /// ESMStore. Insert the reference into the list if a match is
        /// found. If not, throw an exception.
        /// Moved to cpp file, as we require a custom compare operator for it,
        /// and the build will fail with an ugly three-way cyclic header dependence
        /// so we need to pass the instantiation of the method to the linker, when
        /// all methods are known.
        void load (ESM::CellRef &ref, bool deleted, const MWWorld::ESMStore &esmStore);
        void load(ESM4::Reference& ref, bool deleted, const ESMStore& esmStore);
        void load(ESM4::ActorCreature& ref, bool deleted, const ESMStore& esmStore);
        void load(ESM4::ActorCharacter& ref, bool deleted, const ESMStore& esmStore);

        //LiveCellRefBase *find(const ESM4::FormId id)
        LiveCellRefBase *find(const std::string& name)
        //LiveRef *find (const std::string& name)
        {
#if 0
            std::map<ESM4::FormId, std::size_t>::const_iterator iter = mIdMap.find(id);
            if (iter == mIdMap.end())
                return nullptr;

            LiveRef* ref = &mList[iter->second];

            // TODO: keep deleted refs separately?
            // TODO: why return if hasContentFile()?
            if (!ref->mData.isDeletedByContentFile() &&
                (ref->mRef.hasContentFile() || ref->mData.getCount() > 0))
            {
                return ref;
            }

            return nullptr;
#else
            for (typename List::iterator iter (mList.begin()); iter!=mList.end(); ++iter)
                if (!iter->mData.isDeletedByContentFile()
                        && (iter->mRef.hasContentFile() || iter->mData.getCount() > 0)
                        && iter->mRef.getRefId() == name)
                    return &*iter;

            return 0;
#endif
        }

#if 0
        std::vector<LiveCellRefBase*> search(std::int32_t x, std::int32_t y, std::size_t range = 0, std::size_t exclude = 0)
        {
            if (exclude > range)
                throw std::logic_error("The excluded area is larger than the included range.");

            std::vector<LiveCellRefBase*> res;

            for (std::int32_t i = x - std::int32_t(range); i <= x + std::int32_t(range); ++i)
            {
                for (std::int32_t j = y - std::int32_t(range); j <= y + std::int32_t(range); ++j)
                {
                    if (exclude != 0 && i >= x - exclude && i <= x + exclude)
                    {
                        //std::cout << "ignoring x " << i << std::endl; // FIXME
                        continue;
                    }
                    //if (exclude != 0)
                        //std::cout << "processing x" << i << std::endl; // FIXME

                    if (exclude != 0 && j >= y - exclude && j <= y + exclude)
                        continue;

                    GridMap::const_iterator iter = mGridMap.find(std::make_pair(i, j));

                    if (iter != mGridMap.end())
                    {
                        for (std::size_t k = 0; k < iter->second.size(); ++k)
                        {
                            // TODO: check deleted? check count? check hasContentFile()?
                            LiveRef* ref = &mList[iter->second[k]];
                            res.push_back(ref);
                        }
                    }
                }
            }

            return res;
        }
#endif

        LiveRef &insert (const LiveRef& item)
        {
            mList.push_back(item); // HACK
            return mList.back();
        }

        // this is looking for Ogre::SceneNode handles
        LiveCellRefBase *searchViaHandle(const std::string& handle)
        //LiveCellRef<X> *searchViaHandle (const std::string& handle)
        {
            // FIXME: need a handle map?
            for (typename List::iterator iter(mList.begin()); iter != mList.end(); ++iter)
            {
                if (iter->mData.getBaseNode() &&
                    iter->mData.getHandle() == handle) // usually set during Objects::insertBegin()
                {
                    return &*iter;
                }
            }

            return 0;
        }
    };
#endif
}

#endif
