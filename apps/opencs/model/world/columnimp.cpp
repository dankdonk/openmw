#include "columnimp.hpp"

#include <components/esm/loadbody.hpp>

CSMWorld::BodyPartRaceColumn::BodyPartRaceColumn(const ColumnBase *mesh)
    : Column<ESM::BodyPart> (Columns::ColumnId_Race, ColumnBase::Display_Race)
    , mMeshType(static_cast<const MeshTypeColumn<ESM::BodyPart>*>(mesh))
{}

QVariant CSMWorld::BodyPartRaceColumn::get (const Record<ESM::BodyPart>& record) const
{
    if (mMeshType && mMeshType->get(record) == ESM::BodyPart::MT_Skin)
        return QString::fromUtf8 (record.get().mRace.c_str());
    else
        return QVariant(QVariant::UserType);
}

void CSMWorld::BodyPartRaceColumn::set (Record<ESM::BodyPart>& record, const QVariant& data)
{
    ESM::BodyPart record2 = record.get();

    record2.mRace = data.toString().toUtf8().constData();

    record.setModified (record2);
}

bool CSMWorld::BodyPartRaceColumn::isEditable() const
{
    return true;
}
