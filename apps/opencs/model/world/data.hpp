#ifndef CSM_WOLRD_DATA_H
#define CSM_WOLRD_DATA_H

#include <map>
#include <vector>

#include <boost/filesystem/path.hpp>

#include <QObject>
#include <QModelIndex>

#include <components/esm/loadglob.hpp>
#include <components/esm/loadgmst.hpp>
#include <components/esm/loadskil.hpp>
#include <components/esm/loadclas.hpp>
#include <components/esm/loadfact.hpp>
#include <components/esm/loadrace.hpp>
#include <components/esm/loadsoun.hpp>
#include <components/esm/loadscpt.hpp>
#include <components/esm/loadregn.hpp>
#include <components/esm/loadbsgn.hpp>
#include <components/esm/loadspel.hpp>
#include <components/esm/loaddial.hpp>
#include <components/esm/loadench.hpp>
#include <components/esm/loadbody.hpp>
#include <components/esm/loadsndg.hpp>
#include <components/esm/loadmgef.hpp>
#include <components/esm/loadsscr.hpp>
#include <components/esm/debugprofile.hpp>
#include <components/esm/filter.hpp>

#include <components/to_utf8/to_utf8.hpp>

#include "../doc/stage.hpp"

#include "../foreign/navigationcollection.hpp"
#include "../foreign/navmeshcollection.hpp"
#include "../foreign/landcollection.hpp"
#include "../foreign/cellcollection.hpp"
#include "../foreign/landtexturecollection.hpp"
#include "../foreign/worldcollection.hpp"
#include "../foreign/regioncollection.hpp"
#include "../foreign/staticcollection.hpp"
#include "../foreign/refcollection.hpp"
#include "../foreign/charcollection.hpp"
#include "../foreign/idcollection.hpp"
#include "../foreign/animobject.hpp"
#include "../foreign/container.hpp"
#include "../foreign/activator.hpp"
#include "../foreign/miscitem.hpp"
#include "../foreign/Armor.hpp"
#include "../foreign/npc.hpp"
#include "../foreign/flora.hpp"
#include "../foreign/grass.hpp"
#include "../foreign/tree.hpp"
#include "../foreign/light.hpp"
#include "../foreign/book.hpp"
#include "../foreign/furniture.hpp"
#include "../foreign/sound.hpp"
#include "../foreign/weapon.hpp"
#include "../foreign/door.hpp"
#include "../foreign/ammo.hpp"
#include "../foreign/clothing.hpp"
#include "../foreign/potion.hpp"
#include "../foreign/apparatus.hpp"
#include "../foreign/ingredient.hpp"
#include "../foreign/sigilstone.hpp"
#include "../foreign/soulgem.hpp"
#include "../foreign/key.hpp"
#include "../foreign/hair.hpp"
#include "../foreign/eyes.hpp"
#include "../foreign/creature.hpp"
#include "../foreign/leveledcreature.hpp"
#include "../foreign/landtexture.hpp"

#include "idcollection.hpp"
#include "nestedidcollection.hpp"
#include "universalid.hpp"
#include "cell.hpp"
#include "land.hpp"
#include "landtexture.hpp"
#include "refidcollection.hpp"
#include "refcollection.hpp"
#include "infocollection.hpp"
#include "nestedinfocollection.hpp"
#include "pathgrid.hpp"
#include "metadata.hpp"
#ifndef Q_MOC_RUN
#include "subcellcollection.hpp"
#endif

class QAbstractItemModel;

namespace ESM
{
    class ESMReader;
    struct Dialogue;
}

namespace CSMWorld
{
    class ResourcesManager;
    class Resources;
    class NpcAutoCalc;

    class Data : public QObject
    {
            Q_OBJECT

            ToUTF8::Utf8Encoder mEncoder;
            IdCollection<ESM::Global> mGlobals;
            IdCollection<ESM::GameSetting> mGmsts;
            IdCollection<ESM::Skill> mSkills;
            IdCollection<ESM::Class> mClasses;
            NestedIdCollection<ESM::Faction> mFactions;
            NestedIdCollection<ESM::Race> mRaces;
            IdCollection<ESM::Sound> mSounds;
            IdCollection<ESM::Script> mScripts;
            NestedIdCollection<ESM::Region> mRegions;
            NestedIdCollection<ESM::BirthSign> mBirthsigns;
            NestedIdCollection<ESM::Spell> mSpells;
            IdCollection<ESM::Dialogue> mTopics;
            IdCollection<ESM::Dialogue> mJournals;
            NestedIdCollection<ESM::Enchantment> mEnchantments;
            IdCollection<ESM::BodyPart> mBodyParts;
            IdCollection<ESM::MagicEffect> mMagicEffects;
            SubCellCollection<Pathgrid> mPathgrids;
            IdCollection<ESM::DebugProfile> mDebugProfiles;
            IdCollection<ESM::SoundGenerator> mSoundGens;
            IdCollection<ESM::StartScript> mStartScripts;
            NestedInfoCollection mTopicInfos;
            InfoCollection mJournalInfos;
            NestedIdCollection<Cell> mCells;
            IdCollection<LandTexture> mLandTextures;
            IdCollection<Land> mLand;
            RefIdCollection mReferenceables;
            RefCollection mRefs;
            IdCollection<ESM::Filter> mFilters;
            Collection<MetaData> mMetaData;
            CSMForeign::WorldCollection mForeignWorlds;
            CSMForeign::RegionCollection mForeignRegions;
            CSMForeign::CellCollection mForeignCells;
            CSMForeign::LandCollection mForeignLands;
            CSMForeign::IdCollection<CSMForeign::LandTexture> mForeignLandTextures;
            CSMForeign::IdCollection<CSMForeign::Static> mForeignStatics;
            CSMForeign::IdCollection<CSMForeign::AnimObject> mForeignAnimObjs;
            CSMForeign::IdCollection<CSMForeign::Container> mForeignContainers;
            CSMForeign::IdCollection<CSMForeign::MiscItem> mForeignMiscItems;
            CSMForeign::IdCollection<CSMForeign::Activator> mForeignActivators;
            CSMForeign::IdCollection<CSMForeign::Armor> mForeignArmors;
            CSMForeign::IdCollection<CSMForeign::Npc> mForeignNpcs;
            CSMForeign::IdCollection<CSMForeign::Flora> mForeignFloras;
            CSMForeign::IdCollection<CSMForeign::Grass> mForeignGrasses;
            CSMForeign::IdCollection<CSMForeign::Tree> mForeignTrees;
            CSMForeign::IdCollection<CSMForeign::Light> mForeignLights;
            CSMForeign::IdCollection<CSMForeign::Book> mForeignBooks;
            CSMForeign::IdCollection<CSMForeign::Furniture> mForeignFurnitures;
            CSMForeign::IdCollection<CSMForeign::Sound> mForeignSounds;
            CSMForeign::IdCollection<CSMForeign::Weapon> mForeignWeapons;
            CSMForeign::IdCollection<CSMForeign::Door> mForeignDoors;
            CSMForeign::IdCollection<CSMForeign::Ammo> mForeignAmmos;
            CSMForeign::IdCollection<CSMForeign::Clothing> mForeignClothings;
            CSMForeign::IdCollection<CSMForeign::Potion> mForeignPotions;
            CSMForeign::IdCollection<CSMForeign::Apparatus> mForeignApparatuses;
            CSMForeign::IdCollection<CSMForeign::Ingredient> mForeignIngredients;
            CSMForeign::IdCollection<CSMForeign::SigilStone> mForeignSigilStones;
            CSMForeign::IdCollection<CSMForeign::SoulGem> mForeignSoulGems;
            CSMForeign::IdCollection<CSMForeign::Key> mForeignKeys;
            CSMForeign::IdCollection<CSMForeign::Hair> mForeignHairs;
            CSMForeign::IdCollection<CSMForeign::Eyes> mForeignEyesSet;
            CSMForeign::IdCollection<CSMForeign::Creature> mForeignCreatures;
            CSMForeign::IdCollection<CSMForeign::LeveledCreature> mForeignLvlCreatures;
            //CSMForeign::RefIdCollection mForeignReferenceables;
            CSMForeign::RefCollection mForeignRefs;
            CSMForeign::CharCollection mForeignChars;
            CSMForeign::NavigationCollection mNavigation;
            CSMForeign::NavMeshCollection mNavMesh;
            const ResourcesManager& mResourcesManager;
            std::vector<QAbstractItemModel *> mModels;
            std::map<UniversalId::Type, QAbstractItemModel *> mModelIndex;
            std::map<std::string, QAbstractItemModel *> mRegionMapIndex;
            ESM::ESMReader *mReader;
            const ESM::Dialogue *mDialogue; // last loaded dialogue
            bool mBase;
            bool mProject;
            std::map<std::string, std::map<unsigned int, unsigned int> > mRefLoadCache;
            int mReaderIndex;
            std::vector<std::string> mLoadedFiles;

            std::vector<boost::shared_ptr<ESM::ESMReader> > mReaders;

            NpcAutoCalc *mNpcAutoCalc;

            // not implemented
            Data (const Data&);
            Data& operator= (const Data&);

            void addModel (QAbstractItemModel *model, UniversalId::Type type,
                bool update = true);

            static void appendIds (std::vector<std::string>& ids, const CollectionBase& collection,
                bool listDeleted);
            ///< Append all IDs from collection to \a ids.

            static int count (RecordBase::State state, const CollectionBase& collection);

            const Data& self ();

            bool loadTes4Group (CSMDoc::Messages& messages);
            bool loadTes4Record (const ESM4::RecordHeader& hdr, CSMDoc::Messages& messages);

        public:

            Data (ToUTF8::FromType encoding, const ResourcesManager& resourcesManager);

            virtual ~Data();

            const IdCollection<ESM::Global>& getGlobals() const;

            IdCollection<ESM::Global>& getGlobals();

            const IdCollection<ESM::GameSetting>& getGmsts() const;

            IdCollection<ESM::GameSetting>& getGmsts();

            const IdCollection<ESM::Skill>& getSkills() const;

            IdCollection<ESM::Skill>& getSkills();

            const IdCollection<ESM::Class>& getClasses() const;

            IdCollection<ESM::Class>& getClasses();

            const IdCollection<ESM::Faction>& getFactions() const;

            IdCollection<ESM::Faction>& getFactions();

            const IdCollection<ESM::Race>& getRaces() const;

            IdCollection<ESM::Race>& getRaces();

            const IdCollection<ESM::Sound>& getSounds() const;

            IdCollection<ESM::Sound>& getSounds();

            const IdCollection<ESM::Script>& getScripts() const;

            IdCollection<ESM::Script>& getScripts();

            const IdCollection<ESM::Region>& getRegions() const;

            IdCollection<ESM::Region>& getRegions();

            const IdCollection<ESM::BirthSign>& getBirthsigns() const;

            IdCollection<ESM::BirthSign>& getBirthsigns();

            const IdCollection<ESM::Spell>& getSpells() const;

            IdCollection<ESM::Spell>& getSpells();

            const IdCollection<ESM::Dialogue>& getTopics() const;

            IdCollection<ESM::Dialogue>& getTopics();

            const IdCollection<ESM::Dialogue>& getJournals() const;

            IdCollection<ESM::Dialogue>& getJournals();

            const InfoCollection& getTopicInfos() const;

            InfoCollection& getTopicInfos();

            const InfoCollection& getJournalInfos() const;

            InfoCollection& getJournalInfos();

            const IdCollection<Cell>& getCells() const;

            IdCollection<Cell>& getCells();

            const RefIdCollection& getReferenceables() const;

            RefIdCollection& getReferenceables();

            const RefCollection& getReferences() const;

            RefCollection& getReferences();

            const IdCollection<ESM::Filter>& getFilters() const;

            IdCollection<ESM::Filter>& getFilters();

            const IdCollection<ESM::Enchantment>& getEnchantments() const;

            IdCollection<ESM::Enchantment>& getEnchantments();

            const IdCollection<ESM::BodyPart>& getBodyParts() const;

            IdCollection<ESM::BodyPart>& getBodyParts();

            const IdCollection<ESM::DebugProfile>& getDebugProfiles() const;

            IdCollection<ESM::DebugProfile>& getDebugProfiles();

            const IdCollection<CSMWorld::Land>& getLand() const;

            IdCollection<CSMWorld::Land>& getLand();

            const IdCollection<CSMWorld::LandTexture>& getLandTextures() const;

            IdCollection<CSMWorld::LandTexture>& getLandTextures();

            const IdCollection<ESM::SoundGenerator>& getSoundGens() const;

            IdCollection<ESM::SoundGenerator>& getSoundGens();

            const IdCollection<ESM::MagicEffect>& getMagicEffects() const;

            IdCollection<ESM::MagicEffect>& getMagicEffects();

            const SubCellCollection<Pathgrid>& getPathgrids() const;

            SubCellCollection<Pathgrid>& getPathgrids();

            const CSMForeign::WorldCollection& getForeignWorlds() const;

            CSMForeign::WorldCollection& getForeignWorlds();

            const CSMForeign::RegionCollection& getForeignRegions() const;

            CSMForeign::RegionCollection& getForeignRegions();

            const CSMForeign::CellCollection& getForeignCells() const;

            CSMForeign::CellCollection& getForeignCells();

            const CSMForeign::IdCollection<CSMForeign::LandTexture>& getForeignLandTextures() const;

            CSMForeign::IdCollection<CSMForeign::LandTexture>& getForeignLandTextures();

            const CSMForeign::LandCollection& getForeignLands() const;

            CSMForeign::LandCollection& getForeignLands();

            //const CSMForeign::RefIdCollection& getForeignReferenceables() const;

            //CSMForeign::RefIdCollection& getForeignReferenceables();

            const CSMForeign::RefCollection& getForeignReferences() const;

            CSMForeign::RefCollection& getForeignReferences();

            const CSMForeign::CharCollection& getForeignChars() const;

            CSMForeign::CharCollection& getForeignChars();

            const CSMForeign::IdCollection<CSMForeign::Static>& getForeignStatics() const;

            CSMForeign::IdCollection<CSMForeign::Static>& getForeignStatics();

            const CSMForeign::IdCollection<CSMForeign::AnimObject>& getForeignAnimObjs() const;

            CSMForeign::IdCollection<CSMForeign::AnimObject>& getForeignAnimObjs();

            const CSMForeign::IdCollection<CSMForeign::Container>& getForeignContainers() const;

            CSMForeign::IdCollection<CSMForeign::Container>& getForeignContainers();

            const CSMForeign::IdCollection<CSMForeign::MiscItem>& getForeignMiscItems() const;

            CSMForeign::IdCollection<CSMForeign::MiscItem>& getForeignMiscItems();

            const CSMForeign::IdCollection<CSMForeign::Activator>& getForeignActivators() const;

            CSMForeign::IdCollection<CSMForeign::Activator>& getForeignActivators();

            const CSMForeign::IdCollection<CSMForeign::Armor>& getForeignArmors() const;

            CSMForeign::IdCollection<CSMForeign::Armor>& getForeignArmors();

            const CSMForeign::IdCollection<CSMForeign::Npc>& getForeignNpcs() const;

            CSMForeign::IdCollection<CSMForeign::Npc>& getForeignNpcs();

            const CSMForeign::IdCollection<CSMForeign::Flora>& getForeignFloras() const;

            CSMForeign::IdCollection<CSMForeign::Flora>& getForeignFloras();

            const CSMForeign::IdCollection<CSMForeign::Grass>& getForeignGrasses() const;

            CSMForeign::IdCollection<CSMForeign::Grass>& getForeignGrasses();

            const CSMForeign::IdCollection<CSMForeign::Tree>& getForeignTrees() const;

            CSMForeign::IdCollection<CSMForeign::Tree>& getForeignTrees();

            const CSMForeign::IdCollection<CSMForeign::Light>& getForeignLights() const;

            CSMForeign::IdCollection<CSMForeign::Light>& getForeignLights();

            const CSMForeign::IdCollection<CSMForeign::Book>& getForeignBooks() const;

            CSMForeign::IdCollection<CSMForeign::Book>& getForeignBooks();

            const CSMForeign::IdCollection<CSMForeign::Furniture>& getForeignFurnitures() const;

            CSMForeign::IdCollection<CSMForeign::Furniture>& getForeignFurnitures();

            const CSMForeign::IdCollection<CSMForeign::Sound>& getForeignSounds() const;

            CSMForeign::IdCollection<CSMForeign::Sound>& getForeignSounds();

            const CSMForeign::IdCollection<CSMForeign::Weapon>& getForeignWeapons() const;

            CSMForeign::IdCollection<CSMForeign::Weapon>& getForeignWeapons();

            const CSMForeign::IdCollection<CSMForeign::Door>& getForeignDoors() const;

            CSMForeign::IdCollection<CSMForeign::Door>& getForeignDoors();

            const CSMForeign::IdCollection<CSMForeign::Ammo>& getForeignAmmos() const;

            CSMForeign::IdCollection<CSMForeign::Ammo>& getForeignAmmos();

            const CSMForeign::IdCollection<CSMForeign::Clothing>& getForeignClothings() const;

            CSMForeign::IdCollection<CSMForeign::Clothing>& getForeignClothings();

            const CSMForeign::IdCollection<CSMForeign::Potion>& getForeignPotions() const;

            CSMForeign::IdCollection<CSMForeign::Potion>& getForeignPotions();

            const CSMForeign::IdCollection<CSMForeign::Apparatus>& getForeignApparatuses() const;

            CSMForeign::IdCollection<CSMForeign::Apparatus>& getForeignApparatuses();

            const CSMForeign::IdCollection<CSMForeign::Ingredient>& getForeignIngredients() const;

            CSMForeign::IdCollection<CSMForeign::Ingredient>& getForeignIngredients();

            const CSMForeign::IdCollection<CSMForeign::SigilStone>& getForeignSigilStones() const;

            CSMForeign::IdCollection<CSMForeign::SigilStone>& getForeignSigilStones();

            const CSMForeign::IdCollection<CSMForeign::SoulGem>& getForeignSoulGems() const;

            CSMForeign::IdCollection<CSMForeign::SoulGem>& getForeignSoulGems();

            const CSMForeign::IdCollection<CSMForeign::Key>& getForeignKeys() const;

            CSMForeign::IdCollection<CSMForeign::Key>& getForeignKeys();

            const CSMForeign::IdCollection<CSMForeign::Hair>& getForeignHairs() const;

            CSMForeign::IdCollection<CSMForeign::Hair>& getForeignHairs();

            const CSMForeign::IdCollection<CSMForeign::Eyes>& getForeignEyesSet() const;

            CSMForeign::IdCollection<CSMForeign::Eyes>& getForeignEyesSet();

            const CSMForeign::IdCollection<CSMForeign::Creature>& getForeignCreatures() const;

            CSMForeign::IdCollection<CSMForeign::Creature>& getForeignCreatures();

            const CSMForeign::IdCollection<CSMForeign::LeveledCreature>& getForeignLvlCreatures() const;

            CSMForeign::IdCollection<CSMForeign::LeveledCreature>& getForeignLvlCreatures();

            const CSMForeign::NavigationCollection& getNavigation() const;

            CSMForeign::NavigationCollection& getNavigation();

            const CSMForeign::NavMeshCollection& getNavMeshes() const;

            CSMForeign::NavMeshCollection& getNavMeshes();

            const IdCollection<ESM::StartScript>& getStartScripts() const;

            IdCollection<ESM::StartScript>& getStartScripts();

            /// Throws an exception, if \a id does not match a resources list.
            const Resources& getResources (const UniversalId& id) const;

            const MetaData& getMetaData() const;

            void setMetaData (const MetaData& metaData);

            QAbstractItemModel *getTableModel (const UniversalId& id);
            ///< If no table model is available for \a id, an exception is thrown.
            ///
            /// \note The returned table may either be the model for the ID itself or the model that
            /// contains the record specified by the ID.

            void merge();
            ///< Merge modified into base.

            int getTotalRecords (const std::vector<boost::filesystem::path>& files); // for better loading bar

            int startLoading (const boost::filesystem::path& path, bool base, bool project);
            ///< Begin merging content of a file into base or modified.
            ///
            /// \param project load project file instead of content file
            ///
            ///< \return estimated number of records

            bool continueLoading (CSMDoc::Messages& messages);
            ///< \return Finished?

            bool hasId (const std::string& id) const;

            std::vector<std::string> getIds (bool listDeleted = true) const;
            ///< Return a sorted collection of all IDs that are not internal to the editor.
            ///
            /// \param listDeleted include deleted record in the list

            int count (RecordBase::State state) const;
            ///< Return number of top-level records with the given \a state.

            const NpcAutoCalc& getNpcAutoCalc() const;

        signals:

            void idListChanged();

        private slots:

            void dataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void rowsChanged (const QModelIndex& parent, int start, int end);
    };
}

#endif
