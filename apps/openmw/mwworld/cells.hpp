#ifndef GAME_MWWORLD_CELLS_H
#define GAME_MWWORLD_CELLS_H

#include <map>
#include <list>
#include <string>

#include "ptr.hpp"

namespace ESM4
{
    typedef uint32_t FormId;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
    struct CellId;
    struct Cell;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class ESMStore;
    class ForeignWorld;

    /// \brief Cell container
    class Cells
    {
            const MWWorld::ESMStore& mStore;
            std::vector<std::vector<ESM::ESMReader*> >& mReader;
            mutable std::map<std::string, CellStore> mInteriors;
            mutable std::map<std::pair<int, int>, CellStore> mExteriors;
            //
            std::map<std::string, CellStore> mForeignInteriors;
            // FIXME: string or formId?
            // Fortunately, all of TES4 worlds have EditorIds but others may not.
            //mutable std::map<std::string, std::map<std::pair<int, int>, CellStore> > mForeignWorlds;
            std::map<ESM4::FormId, std::map<std::pair<int, int>, CellStore> > mForeignWorlds;
            std::map<ESM4::FormId, CellStore> mForeignDummys;      // key is ForeignWorld FormId
            // FIXME: probably need to be for each cell rather than each world
            std::map<ESM4::FormId, CellStore> mForeignVisibleDist; // key is ForeignWorld FormId
            std::map<std::string, std::vector<std::pair<std::int16_t, std::int16_t> > > mVisibleDistStatics;
            void initNewWorld(const ForeignWorld *world);

            std::vector<std::pair<std::string, CellStore *> > mIdCache;
            std::size_t mIdCacheIndex;

            Cells (const Cells&);
            Cells& operator= (const Cells&);

            CellStore *getCellStore (const ESM::Cell *cell);

            Ptr getPtrAndCache (const std::string& name, CellStore& cellStore);

            void writeCell (ESM::ESMWriter& writer, CellStore& cell) const;

        public:

            void clear();

            Cells (const MWWorld::ESMStore& store, std::vector<std::vector<ESM::ESMReader*> >& reader);

            CellStore *getExterior (int x, int y);

            CellStore *getInterior (const std::string& name);

            CellStore *getCell (const ESM::CellId& id);

            CellStore *getWorldCell (const std::string& world, int x, int y);
            CellStore *getWorldCell (ESM4::FormId worldId, int x, int y);
            CellStore *getWorldDummyCell (ESM4::FormId worldId);
            CellStore *getWorldVisibleDistCell (ESM4::FormId worldId);
            CellStore *getForeignInterior (const std::string& name);

            Ptr getPtr (const std::string& name, CellStore& cellStore, bool searchInContainers = false);
            ///< \param searchInContainers Only affect loaded cells.
            /// @note name must be lower case

            /// @note name must be lower case
            Ptr getPtr (const std::string& name);

            /// Get all Ptrs referencing \a name in exterior cells
            /// @note Due to the current implementation of getPtr this only supports one Ptr per cell.
            /// @note name must be lower case
            void getExteriorPtrs (const std::string& name, std::vector<MWWorld::Ptr>& out);

            /// Get all Ptrs referencing \a name in interior cells
            /// @note Due to the current implementation of getPtr this only supports one Ptr per cell.
            /// @note name must be lower case
            void getInteriorPtrs (const std::string& name, std::vector<MWWorld::Ptr>& out);

            int countSavedGameRecords() const;

            void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;

            bool readRecord (ESM::ESMReader& reader, uint32_t type,
                const std::map<int, int>& contentFileMap);
    };
}

#endif
