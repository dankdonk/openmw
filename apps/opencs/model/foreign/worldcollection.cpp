#include "worldcollection.hpp"

#include <extern/esm4/reader.hpp>

CSMForeign::WorldCollection::WorldCollection ()
{
}

CSMForeign::WorldCollection::~WorldCollection ()
{
}

std::string CSMForeign::WorldCollection::getIdString(std::uint32_t formId) const
{
    int index = searchFormId(formId);
    if (index == -1)
        return "";

    std::string name = getRecord(index).get().mEditorId;
    if (name.empty())
        return "#"+getRecord(index).get().mFullName;
    else
        return name;
}

CSMForeign::World *CSMForeign::WorldCollection::getWorld(ESM4::FormId formId)
{
    int index = searchFormId(formId);
    if (index == -1)
        return nullptr;

    return &getModifiableRecord(index).get();
}
