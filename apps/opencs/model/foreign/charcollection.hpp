#ifndef CSM_FOREIGN_CHARCOLLECTION_H
#define CSM_FOREIGN_CHARCOLLECTION_H

#include <map>

#include "../world/collection.hpp"

#include "cellchar.hpp"

namespace ESM4
{
    class Reader;
    typedef std::uint32_t FormId;
}

namespace CSMWorld
{
    class UniversalId;

    template<>
    void Collection<CSMForeign::CellChar, IdAccessor<CSMForeign::CellChar> >::removeRows (int index, int count);

    template<>
    void Collection<CSMForeign::CellChar, IdAccessor<CSMForeign::CellChar> >::insertRecord (
            std::unique_ptr<RecordBase> record, int index, UniversalId::Type type);
}

// FIXME: this is just a copy of refcollection
namespace CSMForeign
{
    class CellCollection;

    class CharCollection : public CSMWorld::Collection<CellChar, CSMWorld::IdAccessor<CellChar> >
    {
        CSMForeign::CellCollection& mCells;

        typedef std::map<ESM4::FormId, int> RefIndexMap;
        RefIndexMap mRefIndex;

    public:
        CharCollection (CellCollection& cells);
        ~CharCollection ();

        int load(ESM4::Reader& reader, bool base);

        int load (const CellChar& record, bool base, int index = -2);

        virtual void removeRows (int index, int count);

        virtual int searchId (const std::string& id) const;

        virtual void insertRecord (std::unique_ptr<CSMWorld::RecordBase> record,
                                   int index,
                                   CSMWorld::UniversalId::Type type = CSMWorld::UniversalId::Type_None);

        int searchId (ESM4::FormId id) const;

    private:
        CharCollection ();
        CharCollection (const CharCollection& other);
        CharCollection& operator= (const CharCollection& other);

        int getIndex (ESM4::FormId id) const;
    };
}
#endif // CSM_FOREIGN_CHARCOLLECTION_H
