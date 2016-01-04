#ifndef CSM_FOREIGN_TREE_H
#define CSM_FOREIGN_TREE_H

#include <string>

#include <extern/esm4/tree.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Tree : public ESM4::Tree
    {
        static unsigned int sRecordId;

        Tree();
        ~Tree();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_TREE_H
