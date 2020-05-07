#include "foreigndialogue.hpp"

#include <iostream> // FIXME

#include <extern/esm4/reader.hpp>
#include <extern/esm4/info.hpp>

MWWorld::ForeignDialogue::ForeignDialogue()
{
}

MWWorld::ForeignDialogue::~ForeignDialogue()
{
    std::map<ESM4::FormId, ESM4::DialogInfo*>::iterator it = mInfos.begin();
    for (; it != mInfos.end(); ++it)
        delete it->second;
}

void MWWorld::ForeignDialogue::loadInfo(ESM4::Reader& reader, bool merge)
{
    ESM4::DialogInfo* info = new ESM4::DialogInfo;

    info->load(reader);

    // FIXME: should link?  see INFC/INFX and/or INFO::PNAM
    mInfos.insert(std::pair<ESM4::FormId, ESM4::DialogInfo*>(info->mFormId, info));

    //if (std::find(mQuests.begin(), mQuests.end(), info->mQuest) == mQuests.end() &&
        //std::find(mQuests.begin(), mQuests.end(), info->mFormId) == mQuests.end())
        //std::cout << mEditorId << " quest " << ESM4::formIdToString(info->mQuest) << " not found" << std::endl;
    //else
        //std::cout << ESM4::formIdToString(info->mQuest) << " " << info->mResponse << std::endl;
}

void MWWorld::ForeignDialogue::load(ESM4::Reader& reader)
{
    ESM4::Dialogue::load(reader);

    //ESM4::formIdToString(mFormId, mId);
    //ESM4::formIdToString(mParent, mWorldFormId);

    //mName = mFullName;
}

void MWWorld::ForeignDialogue::blank()
{
    // FIXME: TODO
}
