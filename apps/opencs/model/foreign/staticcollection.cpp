#include "staticcollection.hpp"

#include <libs/platform/strings.h>

#include <extern/esm4/reader.hpp>

#include "../world/record.hpp"

#include "worldcollection.hpp"

CSMForeign::StaticCollection::StaticCollection ()
{
}

CSMForeign::StaticCollection::~StaticCollection ()
{
}

int CSMForeign::StaticCollection::load (ESM4::Reader& reader, bool base)
{
    Static record;

    std::string id;
    ESM4::formIdToString(reader.hdr().record.id, id); // use formId converted to string instead

    // FIXME; should be using the formId as the lookup key
    int index = this->searchId(id);

    if (index == -1)
        CSMWorld::IdAccessor<Static>().getId(record) = id;
    else
    {
        record = this->getRecord(index).get();
    }

    loadRecord(record, reader);

    return load(record, base, index);
}

void CSMForeign::StaticCollection::loadRecord (Static& record, ESM4::Reader& reader)
{
    record.load(reader);
}

int CSMForeign::StaticCollection::load (const Static& record, bool base, int index)
{
    if (index == -2)
        index = this->searchId(record.mId);
    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<Static> > record2(new CSMWorld::Record<Static>);

        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<Static> > record2(new CSMWorld::Record<Static>(getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

void CSMForeign::StaticCollection::updateWorldNames (const WorldCollection& worlds)
{
    for (unsigned int i = 0; i < getRecords().size(); ++i)
    {
        std::unique_ptr<CSMWorld::Record<Static> > record(new CSMWorld::Record<Static>(getRecord(i)));
        Static& stat = record->get();
        //stat.mWorld = worlds.getIdString(stat.mWorldId);
        setRecord(i, std::move(record));
    }
}
