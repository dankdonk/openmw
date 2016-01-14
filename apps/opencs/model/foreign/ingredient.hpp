#ifndef CSM_FOREIGN_INGREDIENT_H
#define CSM_FOREIGN_INGREDIENT_H

#include <string>

#include <extern/esm4/ingr.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Ingredient : public ESM4::Ingredient
    {
        static unsigned int sRecordId;

        Ingredient();
        ~Ingredient();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_INGREDIENT_H
