#include "navmeshinfo.hpp"

unsigned int CSMForeign::NavMeshInfo::sRecordId = ESM4::REC_NAVI; // FIXME: not a record but a subrecord

void CSMForeign::NavMeshInfo::load(const ESM4::Navigation::NavMeshInfo& nvmi)
{
    formId           = nvmi.formId;
    flags            = nvmi.flags;
    x                = nvmi.x;
    y                = nvmi.y;
    z                = nvmi.z;
    flagPrefMerges   = nvmi.flagPrefMerges;
    formIdMerged     = nvmi.formIdMerged;
    formIdPrefMerged = nvmi.formIdPrefMerged;
    linkedDoors      = nvmi.linkedDoors;
    islandInfo       = nvmi.islandInfo;
    locationMarker   = nvmi.locationMarker;
    worldSpaceId     = nvmi.worldSpaceId;
    cellGrid         = nvmi.cellGrid;
}

void CSMForeign::NavMeshInfo::blank()
{
    mId              = "";
    formId           = 0;
    flags            = 0;
    x                = 0.f;
    y                = 0.f;
    z                = 0.f;
    flagPrefMerges   = 0;
    formIdMerged.clear();
    formIdPrefMerged.clear();
    linkedDoors.clear();
    islandInfo.clear();
    locationMarker   = 0;
    worldSpaceId     = 0;
    cellGrid.cellId  = 0;
}
