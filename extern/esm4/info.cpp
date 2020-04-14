/*
  Copyright (C) 2020 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#include "info.hpp"

#include <stdexcept>
//#include <iostream> // FIXME: for debugging only

#include "reader.hpp"
#include "formid.hpp" // FIXME: for debugging only
//#include "writer.hpp"

ESM4::DialogInfo::DialogInfo() : mFormId(0), mFlags(0)
{
}

ESM4::DialogInfo::~DialogInfo()
{
}

void ESM4::DialogInfo::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    mEditorId = formIdToString(mFormId); // FIXME: quick workaround to use existing code

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_DATA: // always 3
            case ESM4::SUB_QSTI: // FormId quest id
            case ESM4::SUB_NAME: // FormId add topic
            case ESM4::SUB_TRDT: // 16 bytes
            case ESM4::SUB_NAM1: // response text
            case ESM4::SUB_NAM2: // actor notes
            case ESM4::SUB_CTDT: // older version of CTDA? 20 bytes
            case ESM4::SUB_SCHD: // 28 bytes
            case ESM4::SUB_CTDA: // 24 bytes
            case ESM4::SUB_SCHR: // script data 20 bytes
            case ESM4::SUB_SCDA: // compiled script data 14 bytes
            case ESM4::SUB_TCLT: // FormId choice
            case ESM4::SUB_TCLF: // FormId
            case ESM4::SUB_SCTX: // result script source
            case ESM4::SUB_SCRO: // FormId GLOB reference
            case ESM4::SUB_PNAM: // TES4 DLC
            case ESM4::SUB_TPIC: // TES4 DLC
            case ESM4::SUB_NAM3: // FO3
            case ESM4::SUB_ANAM: // FO3
            case ESM4::SUB_DNAM: // FO3
            case ESM4::SUB_KNAM: // FO3
            case ESM4::SUB_NEXT: // FO3
            case ESM4::SUB_SNDD: // FO3
            case ESM4::SUB_LNAM: // FONV
            case ESM4::SUB_TCFU: // FONV
            case ESM4::SUB_SLSD: // FONV
            case ESM4::SUB_SCRV: // FONV
            case ESM4::SUB_SCVR: // FONV
            case ESM4::SUB_TIFC: // TES5
            case ESM4::SUB_TWAT: // TES5
            case ESM4::SUB_CIS2: // TES5
            case ESM4::SUB_CNAM: // TES5
            case ESM4::SUB_ENAM: // TES5
            case ESM4::SUB_EDID: // TES5
            case ESM4::SUB_VMAD: // TES5
            case ESM4::SUB_BNAM: // TES5
            case ESM4::SUB_SNAM: // TES5
            case ESM4::SUB_ONAM: // TES5
            case ESM4::SUB_QNAM: // TES5
            case ESM4::SUB_RNAM: // TES5
            {
                //std::cout << "INFO " << ESM4::printName(subHdr.typeId) << " skipping..."
                        //<< subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::INFO::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::DialogInfo::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::DialogInfo::blank()
//{
//}
