#ifndef CSM_FOREIGN_REFCOLLECTION_H
#define CSM_FOREIGN_REFCOLLECTION_H

#include <map>

#include "../world/collection.hpp"

#include "cellref.hpp"

namespace ESM4
{
    class Reader;
    typedef std::uint32_t FormId;
}

namespace CSMWorld
{
    class UniversalId;

    template<>
    void Collection<CSMForeign::CellRef, IdAccessor<CSMForeign::CellRef> >::removeRows (int index, int count);

    template<>
    void Collection<CSMForeign::CellRef, IdAccessor<CSMForeign::CellRef> >::insertRecord (
            std::unique_ptr<RecordBase> record, int index, UniversalId::Type type);
}

namespace CSMForeign
{
    class CellCollection;

    class RefCollection : public CSMWorld::Collection<CellRef, CSMWorld::IdAccessor<CellRef> >
    {
        const CSMForeign::CellCollection& mCells; // FIXME: not used, delete?

        typedef std::map<ESM4::FormId, int> RefIndexMap;
        RefIndexMap mRefIndex;

    public:
        RefCollection (const CellCollection& cells);
        ~RefCollection ();

        int load(ESM4::Reader& reader, bool base);

        int load (const CellRef& record, bool base, int index = -2);

        virtual void removeRows (int index, int count);

        virtual int searchId (const std::string& id) const;

        virtual void insertRecord (std::unique_ptr<CSMWorld::RecordBase> record,
                                   int index,
                                   CSMWorld::UniversalId::Type type = CSMWorld::UniversalId::Type_None);

    private:
        RefCollection ();
        RefCollection (const RefCollection& other);
        RefCollection& operator= (const RefCollection& other);

        int getIndex (ESM4::FormId id) const;

        int searchId (ESM4::FormId id) const;
    };
}
#endif // CSM_FOREIGN_REFCOLLECTION_H
