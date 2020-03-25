#include "esmstore.hpp"

#include <set>
#include <iostream>

#include <boost/filesystem/operations.hpp>

#include <extern/esm4/common.hpp>
#include <extern/esm4/formid.hpp>
#include <extern/esm4/refr.hpp>

#include <components/loadinglistener/loadinglistener.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/esm4reader.hpp>

namespace MWWorld
{

static bool isCacheableRecord(int id)
{
    if (id == ESM::REC_ACTI || id == ESM::REC_ALCH || id == ESM::REC_APPA || id == ESM::REC_ARMO ||
        id == ESM::REC_BOOK || id == ESM::REC_CLOT || id == ESM::REC_CONT || id == ESM::REC_CREA ||
        id == ESM::REC_DOOR || id == ESM::REC_INGR || id == ESM::REC_LEVC || id == ESM::REC_LEVI ||
        id == ESM::REC_LIGH || id == ESM::REC_LOCK || id == ESM::REC_MISC || id == ESM::REC_NPC_ ||
        id == ESM::REC_PROB || id == ESM::REC_REPA || id == ESM::REC_STAT || id == ESM::REC_WEAP ||
        id == ESM::REC_BODY)
    {
        return true;
    }
    return false;
}

// FIXME: not sure what records qualify as 'cacheable' - guess is that they are referenceable records
static bool isCacheableForeignRecord(int id)
{
    if (id == MKTAG('N','S','O','U') || /* sound */
        id == MKTAG('I','A','C','T') || /* activator */
        id == MKTAG('A','A','P','P') || /* apparatus */
        id == MKTAG('O','A','R','M') || /* armor */
        id == MKTAG('K','B','O','O') || /* book */
        id == MKTAG('T','C','L','O') || /* clothing */
        id == MKTAG('T','C','O','N') || /* container */
        id == MKTAG('R','D','O','O') || /* door */
        id == MKTAG('R','I','N','G') || /* ingredient */
        id == MKTAG('H','L','I','G') || /* light */
        id == MKTAG('C','M','I','S') || /* misc item */
        id == MKTAG('T','S','T','A') || /* static */
        id == MKTAG('S','G','R','A') || /* grass */
        id == MKTAG('E','T','R','E') || /* tree */
        id == MKTAG('R','F','L','O') || /* flora */
        id == MKTAG('N','F','U','R') || /* furniture */
        id == MKTAG('P','W','E','A') || /* weapon */
        id == MKTAG('O','A','M','M') || /* ammo */
        id == MKTAG('_','N','P','C') || /* npc */
        id == MKTAG('A','C','R','E') || /* creature */
        id == MKTAG('C','L','V','L') || /* lvlcreature */
        id == MKTAG('M','S','L','G') || /* soulgem */
        id == MKTAG('M','K','E','Y') || /* key */
        id == MKTAG('H','A','L','C') || /* potion */
        id == MKTAG('P','S','B','S') || /* subspace */
        id == MKTAG('T','S','G','S') || /* sigilstone */
        id == MKTAG('I','L','V','L') || /* leveled item */
        id == MKTAG('N','L','V','L') || /* leveled actor */
        id == MKTAG('M','I','D','L') || /* idle marker */
        id == MKTAG('T','M','S','T') || /* movable static */
        id == MKTAG('T','T','X','S') || /* texture set */
        id == MKTAG('L','S','C','R') || /* scroll */
        id == MKTAG('A','A','R','M') || /* armature */
        id == MKTAG('T','H','D','P') || /* head part */
        id == MKTAG('M','T','E','R') || /* terminal */
        id == MKTAG('T','T','A','C') || /* talking activator */
        id == MKTAG('E','N','O','T') || /* note */
        id == MKTAG('D','B','P','T')    /* body part */
        )
    {
        return true;
    }
    return false;
}

void ESMStore::load(ESM::ESMReader& esm, Loading::Listener* listener)
{
    listener->setProgressRange(1000);

    ESM::Dialogue *dialogue = 0;

    int esmVer = esm.getVer();
    bool isTes4 = esmVer == ESM::VER_080 || esmVer == ESM::VER_100;
    bool isTes5 = esmVer == ESM::VER_094 || esmVer == ESM::VER_17;
    bool isFONV = esmVer == ESM4::VER_132 || esmVer == ESM4::VER_133 || esmVer == ESM4::VER_134;

    // FIXME: temporary workaround
    if (!(isTes4 || isTes5 || isFONV)) // MW only
    {
        // Land texture loading needs to use a separate internal store for each plugin.
        // We set the number of plugins here to avoid continual resizes during loading,
        // and so we can properly verify if valid plugin indices are being passed to the
        // LandTexture Store retrieval methods.
        mLandTextures.resize(esm.getGlobalReaderList()->size()); // FIXME: size should be for MW only
    }

    // FIXME: for TES4/TES5 whether a dependent file is loaded is already checked in
    // ESM4::Reader::updateModIndicies() which is called in EsmLoader::load() before this
    if (!(isTes4 || isTes5 || isFONV)) // MW only
    {
        /// \todo Move this to somewhere else. ESMReader?
        // Cache parent esX files by tracking their indices in the global list of
        //  all files/readers used by the engine. This will greaty accelerate
        //  refnumber mangling, as required for handling moved references.
        const std::vector<ESM::Header::MasterData> &masters = esm.getGameFiles();
        std::vector<ESM::ESMReader*> *allPlugins = esm.getGlobalReaderList();
        for (size_t j = 0; j < masters.size(); j++) {
            ESM::Header::MasterData &mast = const_cast<ESM::Header::MasterData&>(masters[j]);
            std::string fname = mast.name;
            int index = ~0;
            for (int i = 0; i < esm.getIndex(); i++) {
                const std::string &candidate = allPlugins->at(i)->getContext().filename;
                std::string fnamecandidate = boost::filesystem::path(candidate).filename().string();
                if (Misc::StringUtils::ciEqual(fname, fnamecandidate)) {
                    index = i;
                    break;
                }
            }
            if (index == (int)~0) {
                // Tried to load a parent file that has not been loaded yet. This is bad,
                //  the launcher should have taken care of this.
                std::string fstring = "File " + esm.getName() + " asks for parent file " + masters[j].name
                    + ", but it has not been loaded yet. Please check your load order.";
                esm.fail(fstring);
            }
            mast.index = index;
        }
    }

    // Loop through all records
    while(esm.hasMoreRecs())
    {
        if (isTes4 || isTes5 || isFONV)
        {
            ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
            reader.checkGroupStatus();

            loadTes4Group(esm);
            listener->setProgress(static_cast<size_t>(esm.getFileOffset() / (float)esm.getFileSize() * 1000));
            continue;
        }

        ESM::NAME n = esm.getRecName();
        esm.getRecHeader();

        // Look up the record type.
        std::map<int, StoreBase *>::iterator it = mStores.find(n.val);

        if (it == mStores.end()) {
            if (n.val == ESM::REC_INFO) {
                if (dialogue)
                {
                    dialogue->readInfo(esm, esm.getIndex() != 0);
                }
                else
                {
                    std::cerr << "error: info record without dialog" << std::endl;
                    esm.skipRecord();
                }
            } else if (n.val == ESM::REC_MGEF) {
                mMagicEffects.load (esm);
            } else if (n.val == ESM::REC_SKIL) {
                mSkills.load (esm);
            }
            else if (n.val==ESM::REC_FILT || n.val == ESM::REC_DBGP)
            {
                // ignore project file only records
                esm.skipRecord();
            }
            else {
                std::stringstream error;
                error << "Unknown record: " << n.toString();
                throw std::runtime_error(error.str());
            }
        } else {
            RecordId id = it->second->load(esm);
            if (id.mIsDeleted)
            {
                it->second->eraseStatic(id.mId);
                continue;
            }

            if (n.val==ESM::REC_DIAL) {
                dialogue = const_cast<ESM::Dialogue*>(mDialogs.find(id.mId));
            } else {
                dialogue = 0;
            }
        }
        listener->setProgress(static_cast<size_t>(esm.getFileOffset() / (float)esm.getFileSize() * 1000));
    }
}

// Can't use ESM4::Reader& as the parameter here because we need esm.hasMoreRecs() for
// checking an empty group followed by EOF
void ESMStore::loadTes4Group (ESM::ESMReader &esm)
{
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

    reader.getRecordHeader();
    const ESM4::RecordHeader& hdr = reader.hdr();

    if (hdr.record.typeId != ESM4::REC_GRUP)
        return loadTes4Record(esm);

    switch (hdr.group.type)
    {
        case ESM4::Grp_RecordType:
        {
            // FIXME: rewrite to workaround reliability issue
            if (hdr.group.label.value == ESM4::REC_NAVI || hdr.group.label.value == ESM4::REC_WRLD ||
                hdr.group.label.value == ESM4::REC_REGN || hdr.group.label.value == ESM4::REC_STAT ||
                hdr.group.label.value == ESM4::REC_ANIO || hdr.group.label.value == ESM4::REC_CONT ||
                hdr.group.label.value == ESM4::REC_MISC || hdr.group.label.value == ESM4::REC_ACTI ||
                hdr.group.label.value == ESM4::REC_ARMO || hdr.group.label.value == ESM4::REC_NPC_ ||
                hdr.group.label.value == ESM4::REC_FLOR || hdr.group.label.value == ESM4::REC_GRAS ||
                hdr.group.label.value == ESM4::REC_TREE || hdr.group.label.value == ESM4::REC_LIGH ||
                hdr.group.label.value == ESM4::REC_BOOK || hdr.group.label.value == ESM4::REC_FURN ||
                hdr.group.label.value == ESM4::REC_SOUN || hdr.group.label.value == ESM4::REC_WEAP ||
                hdr.group.label.value == ESM4::REC_DOOR || hdr.group.label.value == ESM4::REC_AMMO ||
                hdr.group.label.value == ESM4::REC_CLOT || hdr.group.label.value == ESM4::REC_ALCH ||
                hdr.group.label.value == ESM4::REC_APPA || hdr.group.label.value == ESM4::REC_INGR ||
                hdr.group.label.value == ESM4::REC_SGST || hdr.group.label.value == ESM4::REC_SLGM ||
                hdr.group.label.value == ESM4::REC_KEYM || hdr.group.label.value == ESM4::REC_HAIR ||
                hdr.group.label.value == ESM4::REC_EYES || hdr.group.label.value == ESM4::REC_CELL ||
                hdr.group.label.value == ESM4::REC_CREA || hdr.group.label.value == ESM4::REC_LVLC ||
                hdr.group.label.value == ESM4::REC_LVLI || hdr.group.label.value == ESM4::REC_MATO ||
                hdr.group.label.value == ESM4::REC_IDLE || hdr.group.label.value == ESM4::REC_LTEX ||
                hdr.group.label.value == ESM4::REC_RACE || hdr.group.label.value == ESM4::REC_SBSP ||
                hdr.group.label.value == ESM4::REC_LVLN || hdr.group.label.value == ESM4::REC_IDLM ||
                hdr.group.label.value == ESM4::REC_MSTT || hdr.group.label.value == ESM4::REC_TXST ||
                hdr.group.label.value == ESM4::REC_SCRL || hdr.group.label.value == ESM4::REC_ARMA ||
                hdr.group.label.value == ESM4::REC_HDPT || hdr.group.label.value == ESM4::REC_TERM ||
                hdr.group.label.value == ESM4::REC_TACT || hdr.group.label.value == ESM4::REC_NOTE ||
                hdr.group.label.value == ESM4::REC_BPTD || hdr.group.label.value == ESM4::REC_SCPT ||
                hdr.group.label.value == ESM4::REC_DIAL || hdr.group.label.value == ESM4::REC_QUST ||
                hdr.group.label.value == ESM4::REC_INFO || hdr.group.label.value == ESM4::REC_PACK
                )
            {
                reader.saveGroupStatus();
                loadTes4Group(esm);
            }
            else
            {
                // Skip groups that are of no interest (for now).
                //  GMST GLOB CLAS FACT SKIL MGEF ENCH SPEL BSGN WTHR CLMT
                //  CSTY LSCR LVSP WATR EFSH

                // FIXME: The label field of a group is not reliable, so we will need to check here as well
                std::cout << "skipping group... " << ESM4::printLabel(hdr.group.label, hdr.group.type) << std::endl;
                reader.skipGroup();
                return;
            }

            break;
        }
        case ESM4::Grp_CellChild:
        case ESM4::Grp_WorldChild:
        case ESM4::Grp_TopicChild:
        case ESM4::Grp_CellPersistentChild:
        {
            reader.adjustGRUPFormId();  // not needed or even shouldn't be done? (only labels anyway)
            reader.saveGroupStatus();
//#if 0
            // Below test shows that Oblivion.esm does not have any persistent cell child
            // groups under exterior world sub-block group.  Haven't checked other files yet.
             if (reader.grp(0).type == ESM4::Grp_CellPersistentChild &&
                 reader.grp(1).type == ESM4::Grp_CellChild &&
                 !(reader.grp(2).type == ESM4::Grp_WorldChild || reader.grp(2).type == ESM4::Grp_InteriorSubCell))
                 std::cout << "Unexpected persistent child group in exterior subcell" << std::endl;
//#endif
            if (!esm.hasMoreRecs())
                return; // may have been an empty group followed by EOF

            // If hdr.group.type == ESM4::Grp_CellPersistentChild, we are about to
            // partially load the persistent records such as REFR, ACHR and ACRE, if any.
            // (well, actually only examining for doors for the moment)
            //
            // The records from other groups are skipped as per below.
            loadTes4Group(esm);

            break;
        }
        case ESM4::Grp_CellTemporaryChild:
        case ESM4::Grp_CellVisibleDistChild:
        {
            // NOTE: preload strategy and persistent records
            //
            // Current strategy defers loading of "temporary" or "visible when distant"
            // references and other records (land and pathgrid) until they are needed.
            //
            // The "persistent" records need to be loaded up front, however.  This is to allow,
            // for example, doors to work.  A door reference will have a FormId of the
            // destination door FormId.  But we have no way of knowing to which cell the
            // destination FormId belongs until that cell and that reference is loaded.
            //
            // For worldspaces the persistent records are usully (always?) stored in a dummy
            // cell under a "world child" group.  It may be possible to skip the whole "cell
            // child" group without scanning for persistent records.  See above short test.
            reader.skipGroup();
            break;
        }
        case ESM4::Grp_ExteriorCell:
        case ESM4::Grp_ExteriorSubCell:
        case ESM4::Grp_InteriorCell:
        case ESM4::Grp_InteriorSubCell:
        {
            reader.saveGroupStatus();
            loadTes4Group(esm);

            break;
        }
        default:
            std::cout << "unknown group..." << std::endl; // FIXME, should throw?
            reader.skipGroup();
            break;
    }

    return;
}

void ESMStore::loadTes4Record (ESM::ESMReader& esm)
{
    // Assumes that the reader has just read the record header only.
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
    const ESM4::RecordHeader& hdr = reader.hdr();

    switch (hdr.record.typeId)
    {
        // GMST, GLOB, CLAS, FACT
        case ESM4::REC_HAIR: reader.getRecordData(); mForeignHairs.load(esm); break;
        case ESM4::REC_EYES: reader.getRecordData(); mForeignEyesSet.load(esm); break;
        case ESM4::REC_RACE: reader.getRecordData(); mForeignRaces.load(esm); break;
        case ESM4::REC_SOUN: reader.getRecordData(); mForeignSounds.load(esm); break;
		// SKIL, MGEF
        case ESM4::REC_LTEX: reader.getRecordData(); mForeignLandTextures.load(esm); break;
        case ESM4::REC_SCPT: reader.getRecordData(); mForeignScripts.load(esm); break;
		// ENCH, SPEL, BSGN
        // ---- referenceables start
        case ESM4::REC_ACTI: reader.getRecordData(); mForeignActivators.load(esm); break;
        case ESM4::REC_APPA: reader.getRecordData(); mForeignApparatuses.load(esm); break;
        case ESM4::REC_ARMO: reader.getRecordData(); mForeignArmors.load(esm); break;
        case ESM4::REC_BOOK: reader.getRecordData(); mForeignBooks.load(esm); break;
        case ESM4::REC_CLOT: reader.getRecordData(); mForeignClothes.load(esm); break;
        case ESM4::REC_CONT: reader.getRecordData(); mForeignContainers.load(esm); break;
        case ESM4::REC_DOOR: reader.getRecordData(); mForeignDoors.load(esm); break;
        case ESM4::REC_INGR: reader.getRecordData(); mForeignIngredients.load(esm); break;
        case ESM4::REC_LIGH: reader.getRecordData(); mForeignLights.load(esm); break;
        case ESM4::REC_MISC: reader.getRecordData(); mForeignMiscItems.load(esm); break;
        case ESM4::REC_STAT: reader.getRecordData(); mForeignStatics.load(esm); break;
        case ESM4::REC_GRAS: reader.getRecordData(); mForeignGrasses.load(esm); break;
        case ESM4::REC_TREE: reader.getRecordData(); mForeignTrees.load(esm); break;
        case ESM4::REC_FLOR: reader.getRecordData(); mForeignFloras.load(esm); break;
        case ESM4::REC_FURN: reader.getRecordData(); mForeignFurnitures.load(esm); break;
        case ESM4::REC_WEAP: reader.getRecordData(); mForeignWeapons.load(esm); break;
        case ESM4::REC_AMMO: reader.getRecordData(); mForeignAmmos.load(esm); break;
        // UrielSeptim
        //case ESM4::REC_NPC_: reader.getRecordData(((hdr.record.id & 0xfffff) == 0x23F2E)?true:false); mForeignNpcs.load(esm); break;
        case ESM4::REC_NPC_: reader.getRecordData(); mForeignNpcs.load(esm); break;
        case ESM4::REC_CREA: reader.getRecordData(); mForeignCreatures.load(esm); break;
        case ESM4::REC_LVLC: reader.getRecordData(); mForeignLvlCreatures.load(esm); break;
        case ESM4::REC_SLGM: reader.getRecordData(); mForeignSoulGems.load(esm); break;
        case ESM4::REC_KEYM: reader.getRecordData(); mForeignKeys.load(esm); break;
        case ESM4::REC_ALCH: reader.getRecordData(); mForeignPotions.load(esm); break;
        case ESM4::REC_SBSP: reader.getRecordData(); mForeignSubspaces.load(esm); break;
        case ESM4::REC_SGST: reader.getRecordData(); mForeignSigilStones.load(esm); break;
        case ESM4::REC_LVLI: reader.getRecordData(); mForeignLvlItems.load(esm); break;
        case ESM4::REC_LVLN: reader.getRecordData(); mForeignLvlActors.load(esm); break;
        case ESM4::REC_IDLM: reader.getRecordData(); mForeignIdleMarkers.load(esm); break;
        case ESM4::REC_MSTT: reader.getRecordData(); mForeignMovableStatics.load(esm); break;
        case ESM4::REC_TXST: reader.getRecordData(); mForeignTextureSets.load(esm); break;
        case ESM4::REC_SCRL: reader.getRecordData(); mForeignScrolls.load(esm); break;
        case ESM4::REC_ARMA: reader.getRecordData(); mForeignArmorAddons.load(esm); break;
        case ESM4::REC_HDPT: reader.getRecordData(); mForeignHeadParts.load(esm); break;
        case ESM4::REC_TERM: reader.getRecordData(); mForeignTerminals.load(esm); break;
        case ESM4::REC_TACT: reader.getRecordData(); mForeignTalkingActivators.load(esm); break;
        case ESM4::REC_NOTE: reader.getRecordData(); mForeignNotes.load(esm); break;
        case ESM4::REC_BPTD: reader.getRecordData(); mForeignBodyParts.load(esm); break;
        // ---- referenceables end
        // WTHR, CLMT
        //case ESM4::REC_REGN: reader.getRecordData(); mForeignRegions.load(esm); break;
        case ESM4::REC_CELL:
        {
            // do not load and just save context
            mForeignCells.preload(esm, mForeignWorlds);
            // FIXME: deal with deleted recods

            // FIXME testing only - uncomment below call to testPreload() to test
            // saving/restoring file contexts
            // also see ForeignCell::testPreload() and Store<MWWorld::ForeignCell>::testPreload()
            //mForeignCells.testPreload(esm);
            break;
        }
        case ESM4::REC_WRLD:
        {
            RecordId id = mForeignWorlds.load(esm);
            if (id.mIsDeleted)
                mForeignWorlds.eraseStatic(id.mId);

            // will be followed by another CELL or a Cell Child GRUP
            break;
        }
        case ESM4::REC_DIAL: reader.getRecordData(); mForeignDialogs.load(esm);     break;
        case ESM4::REC_INFO: reader.getRecordData(); mForeignDialogInfos.load(esm); break;
        case ESM4::REC_QUST: reader.getRecordData(); mForeignQuests.load(esm);      break;
        // IDLE
        // CSTY, LSCR, LVSP
        case ESM4::REC_PACK: reader.getRecordData(); mForeignAIPackages.load(esm);  break;
        case ESM4::REC_ANIO: reader.getRecordData(); mForeignAnimObjs.load(esm);    break;
        // WATR, EFSH
        case ESM4::REC_REFR:
        {
            // this REFR must be in "Cell Persistent Child" group
            ESM4::Reference record;

            reader.getRecordData();
            record.load(reader);

            // FIXME: loading *all* references just to check for doors is highly inefficient
            if (record.mDoor.destDoor != 0)
            {
                std::pair<std::map<ESM4::FormId, ESM4::FormId>::iterator, bool> result
                    = mDoorDestCell.insert({ record.mFormId, reader.currCell() });

                // FIXME: detect duplicates?
            }

            break;
        }
        case ESM4::REC_ACHR:
        {
            // this ACHR must be in "Cell Persistent Child" group
            reader.skipRecordData();
            break;
        }
        case ESM4::REC_ACRE: // Oblivion only?
        {
            // this ACHE must be in "Cell Persistent Child" group
            reader.skipRecordData();
            break;
        }
#if 0
        // NOTE: LAND records are loaded later (for now) - see CellStore
        // not loaded here since LAND is in "Cell Temporary Child" group
        // TODO: verify LTEX formIds exist
        case ESM4::REC_LAND: reader.getRecordData(); mForeignLands.load(esm, mForeignCells); break;
        case ESM4::REC_NAVI: reader.getRecordData(); mNavigation.load(esm); break;
        case ESM4::REC_NAVM:
        {
            // FIXME: should update mNavMesh to indicate this record was deleted
            if ((hdr.record.flags & ESM4::Rec_Deleted) != 0)
            {
//FIXME: debug only
#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "NAVM: deleted record id "
                          << std::hex << hdr.record.id << std::endl;
#endif
                reader.skipRecordData();
                break;
            }

            reader.getRecordData();
            mNavMesh.load(esm);
            break;
        }
		//
        // PGRD is handled in CellStore::loadTes4Record()
        // not loaded here since PGRD is in "Cell Temporary Child" group
        case ESM4::REC_PGRD: // Oblivion only?
		//
        case ESM4::REC_IDLE:
        case ESM4::REC_MATO:
		//
        case ESM4::REC_PHZD:
        case ESM4::REC_PGRE:
        case ESM4::REC_ROAD: // Oblivion only?
        {
            //std::cout << ESM4::printName(hdr.record.typeId) << " skipping..." << std::endl;
            reader.skipRecordData();
            break;
        }
#endif
        case ESM4::REC_REGN:
        case ESM4::REC_PHZD: case ESM4::REC_PGRE: // Skyrim only?
        case ESM4::REC_ROAD: case ESM4::REC_NAVM: case ESM4::REC_NAVI:
        case ESM4::REC_IDLE:
        case ESM4::REC_MATO:
        {
            //std::cout << ESM4::printName(hdr.record.typeId) << " skipping..." << std::endl;
            reader.skipRecordData();
            break;
        }
        default:
        {
            std::cout << "Unsupported TES4 record type: " + ESM4::printName(hdr.record.typeId) << std::endl;
            reader.skipRecordData();
        }
    }

    return;
}

void ESMStore::setUp()
{
    mIds.clear();
    mForeignIds.clear();

    std::map<int, StoreBase *>::iterator storeIt = mStores.begin();
    for (; storeIt != mStores.end(); ++storeIt)
    {
        storeIt->second->setUp();

        if (isCacheableRecord(storeIt->first))
        {
            std::vector<std::string> identifiers;
            storeIt->second->listIdentifier(identifiers);

            for (std::vector<std::string>::const_iterator record = identifiers.begin(); record != identifiers.end(); ++record)
                mIds[*record] = storeIt->first; // FIXME: log duplicates?
        }
        else if (isCacheableForeignRecord(storeIt->first))
        {
            std::vector<ESM4::FormId> identifiers;
            storeIt->second->listForeignIdentifier(identifiers);

            for (std::vector<ESM4::FormId>::const_iterator record = identifiers.begin(); record != identifiers.end(); ++record)
                mForeignIds[*record] = storeIt->first; // FIXME: log duplicates?
        }
    }
    mSkills.setUp();
    mMagicEffects.setUp();
    mAttributes.setUp();
    mDialogs.setUp();
}

    int ESMStore::countSavedGameRecords() const
    {
        return 1 // DYNA (dynamic name counter)
            +mPotions.getDynamicSize()
            +mArmors.getDynamicSize()
            +mBooks.getDynamicSize()
            +mClasses.getDynamicSize()
            +mClothes.getDynamicSize()
            +mEnchants.getDynamicSize()
            +mNpcs.getDynamicSize()
            +mSpells.getDynamicSize()
            +mWeapons.getDynamicSize()
            +mCreatureLists.getDynamicSize()
            +mItemLists.getDynamicSize();
    }

    void ESMStore::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
    {
        writer.startRecord(ESM::REC_DYNA);
        writer.startSubRecord("COUN");
        writer.writeT(mDynamicCount);
        writer.endRecord("COUN");
        writer.endRecord(ESM::REC_DYNA);

        mPotions.write (writer, progress);
        mArmors.write (writer, progress);
        mBooks.write (writer, progress);
        mClasses.write (writer, progress);
        mClothes.write (writer, progress);
        mEnchants.write (writer, progress);
        mSpells.write (writer, progress);
        mWeapons.write (writer, progress);
        mNpcs.write (writer, progress);
        mItemLists.write (writer, progress);
        mCreatureLists.write (writer, progress);
    }

    bool ESMStore::readRecord (ESM::ESMReader& reader, uint32_t type)
    {
        switch (type)
        {
            case ESM::REC_ALCH:
            case ESM::REC_ARMO:
            case ESM::REC_BOOK:
            case ESM::REC_CLAS:
            case ESM::REC_CLOT:
            case ESM::REC_ENCH:
            case ESM::REC_SPEL:
            case ESM::REC_WEAP:
            case ESM::REC_NPC_:
            case ESM::REC_LEVI:
            case ESM::REC_LEVC:

                {
                    mStores[type]->read (reader);
                }

                if (type==ESM::REC_NPC_)
                {
                    // NPC record will always be last and we know that there can be only one
                    // dynamic NPC record (player) -> We are done here with dynamic record loading
                    setUp();

                    const ESM::NPC *player = mNpcs.find ("player");

                    if (!mRaces.find (player->mRace) ||
                        !mClasses.find (player->mClass))
                        throw std::runtime_error ("Invalid player record (race or class unavailable");
                }

                return true;

            case ESM::REC_DYNA:
                reader.getSubNameIs("COUN");
                reader.getHT(mDynamicCount);
                return true;

            default:

                return false;
        }
    }

} // end namespace
