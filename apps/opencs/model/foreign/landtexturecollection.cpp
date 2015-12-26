#include "landtexturecollection.hpp"

#include <iostream> // FIXME

#include <libs/platform/strings.h>

#include <extern/esm4/reader.hpp>

#include "../world/record.hpp"

CSMForeign::LandTextureCollection::LandTextureCollection ()
{
}

CSMForeign::LandTextureCollection::~LandTextureCollection ()
{
}

// Can't reliably use cell grid as the id for LAND, since some cells can be "empty" or do not
// have an XCLC sub-record. e.g. OblivionMQKvatchBridge, TheFrostFireGlade and CheydinhalOblivion
int CSMForeign::LandTextureCollection::load (ESM4::Reader& reader, bool base)
{
    LandTexture record;

    std::string id;
#if 0
    if (reader.hasCellGrid())
    {
        std::ostringstream stream;
        stream << "#" << reader.currCellGrid().grid.x << " " << reader.currCellGrid().grid.y;
        id = stream.str();

        std::string padding = "";
        padding.insert(0, reader.stackSize()*2, ' ');
        std::cout << padding << "LAND: formId " << std::hex << reader.hdr().record.id << std::endl; // FIXME
        std::cout << padding << "LAND X " << std::dec << reader.currCellGrid().grid.x << ", Y " << reader.currCellGrid().grid.y << std::endl;
    }
    else
        id = std::to_string(reader.hdr().record.id); // use formId instead
#endif

    record.mName = id; // FIXME: temporary, note id overwritten below

    //id = std::to_string(reader.hdr().record.id); // use formId converted to string instead
    char buf[8+1];
    int res = snprintf(buf, 8+1, "%08x", reader.hdr().record.id);
    if (res > 0 && res < 100)
        id.assign(buf);
    else
        throw std::runtime_error("Landscape Texture Collection possible buffer overflow on formId");

    // FIXME; should be using the formId as the lookup key
    int index = this->searchId(id);

    if (index == -1)
        CSMWorld::IdAccessor<LandTexture>().getId(record) = id;
    else
    {
        record = this->getRecord(index).get();
    }

    loadRecord(record, reader);

    return load(record, base, index);
}

void CSMForeign::LandTextureCollection::loadRecord (LandTexture& record, ESM4::Reader& reader)
{
    record.load(reader);
}

int CSMForeign::LandTextureCollection::load (const LandTexture& record, bool base, int index)
{
    if (index == -2)
        index = this->searchId(CSMWorld::IdAccessor<LandTexture>().getId(record));

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<LandTexture> > record2(new CSMWorld::Record<LandTexture>);

        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<LandTexture> > record2(new CSMWorld::Record<LandTexture>(
                    CSMWorld::Collection<LandTexture, CSMWorld::IdAccessor<LandTexture> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}
