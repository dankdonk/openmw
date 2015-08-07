#ifndef CSM_FOREIGN_NAVIGATIONCOLLECTION_H
#define CSM_FOREIGN_NAVIGATIONCOLLECTION_H

#include "../world/collection.hpp" // vector, map

#include "navmeshinfo.hpp" // string

namespace ESM4
{
    class Reader;
}

namespace CSMWorld
{
    struct Cell;

    template<typename AT>
    struct IdAccessor;

    template<typename T, typename AT>
    class IdCollection;
}

namespace CSMForeign
{
    class NavigationCollection : public CSMWorld::Collection<NavMeshInfo, CSMWorld::IdAccessor<NavMeshInfo> >//, public NestedCollection
    {
        const CSMWorld::IdCollection<CSMWorld::Cell, CSMWorld::IdAccessor<CSMWorld::Cell> >& mCells;
        ESM4::Navigation mNavigation;

        std::map<std::string, std::vector<std::string> > mCellToFormIds;

    public:
        NavigationCollection (const CSMWorld::IdCollection<CSMWorld::Cell, CSMWorld::IdAccessor<CSMWorld::Cell> >& cells);
        ~NavigationCollection ();

        // similar to IdCollection but with ESM4::Reader
        void load (ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const NavMeshInfo& record, bool base, int index = -2);

        const std::map<std::string, std::vector<std::string> >& cellToFormIds () const {
            return mCellToFormIds; }

        //virtual void loadRecord (NavMeshInfo& record, ESM4::Reader& reader);

    private:
        NavigationCollection ();
        NavigationCollection (const NavigationCollection& other);
        NavigationCollection& operator= (const NavigationCollection& other);
    };
#if 0
    class NavigationCollection : public Collection<ESM4::Navigation>
    {
        ESM4::Navigation mNavigation;

    public:

        NavigationCollection ();
        ~NavigationCollection ();

        void load (ESM4::Reader& reader, bool base);

    private:
        NavigationCollection (const NavigationCollection& other);
        NavigationCollection& operator= (const NavigationCollection& other);
    };
#endif
}

#endif // CSM_FOREIGN_NAVIGATIONCOLLECTION_H
