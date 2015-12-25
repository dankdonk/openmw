#include "worldcollection.hpp"

#include <libs/platform/strings.h>

#include <extern/esm4/reader.hpp>

#include "../world/record.hpp"

CSMForeign::WorldCollection::WorldCollection ()
{
}

CSMForeign::WorldCollection::~WorldCollection ()
{
}

int CSMForeign::WorldCollection::load (ESM4::Reader& reader, bool base)
{
    CSMForeign::World record;

    std::string id;
    //id = std::to_string(reader.hdr().record.id); // use formId converted to string instead
    char buf[8+1];
    int res = snprintf(buf, 8+1, "%08x", reader.hdr().record.id);
    if (res > 0 && res < 100)
        id.assign(buf);
    else
        throw std::runtime_error("World Collection possible buffer overflow on formId");

    // FIXME; should be using the formId as the lookup key
    int index = this->searchId(id);

    if (index == -1)
        CSMWorld::IdAccessor<CSMForeign::World>().getId(record) = id;
    else
    {
        record = this->getRecord(index).get();
    }

    loadRecord(record, reader);

    return load(record, base, index);
}

void CSMForeign::WorldCollection::loadRecord (CSMForeign::World& record,
    ESM4::Reader& reader)
{
    record.load(reader);
}

int CSMForeign::WorldCollection::load (const CSMForeign::World& record, bool base, int index)
{
    if (index == -2)
        index = this->searchId(CSMWorld::IdAccessor<CSMForeign::World>().getId(record));

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<CSMForeign::World> > record2(
                new CSMWorld::Record<CSMForeign::World>);

        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<CSMForeign::World> > record2(
                new CSMWorld::Record<CSMForeign::World>(
                    CSMWorld::Collection<CSMForeign::World, CSMWorld::IdAccessor<CSMForeign::World> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

std::string CSMForeign::WorldCollection::getIdString(std::uint32_t formId) const
{
    std::string id;// = std::to_string(formId);
    char buf[8+1];
    int res = snprintf(buf, 8+1, "%08x", formId);
    if (res > 0 && res < 100)
        id.assign(buf);
    else
        throw std::runtime_error("World Collection possible buffer overflow on formId");

    int index = searchId(id);
    if (index == -1)
        return "";

    std::string name = getRecord(index).get().mEditorId;
    if (name.empty())
        return "#"+getRecord(index).get().mFullName;
    else
        return name;
}
