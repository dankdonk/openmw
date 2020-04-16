#ifndef OPENMW_MECHANICS_LEVELLEDLIST_H
#define OPENMW_MECHANICS_LEVELLEDLIST_H

#include <components/misc/rng.hpp>
#include <extern/esm4/common.hpp>
#include <extern/esm4/formid.hpp>

#include <iostream>


#include "../mwworld/ptr.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/class.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwmechanics/creaturestats.hpp"

namespace MWMechanics
{

    /// @return ID of resulting item, or empty if none
    inline std::string getLevelledItem (const ESM::LevelledListBase* levItem, bool creature, unsigned char failChance=0)
    {
        const std::vector<ESM::LevelledListBase::LevelItem>& items = levItem->mList;

        const MWWorld::Ptr& player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        int playerLevel = player.getClass().getCreatureStats(player).getLevel();

        failChance += levItem->mChanceNone;

        if (Misc::Rng::roll0to99() < failChance)
            return std::string();

        std::vector<std::string> candidates;
        int highestLevel = 0;
        for (std::vector<ESM::LevelledListBase::LevelItem>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (it->mLevel > highestLevel && it->mLevel <= playerLevel)
                highestLevel = it->mLevel;
        }

        // For levelled creatures, the flags are swapped. This file format just makes so much sense.
        bool allLevels = (levItem->mFlags & ESM::ItemLevList::AllLevels) != 0;
        if (creature)
            allLevels = levItem->mFlags & ESM::CreatureLevList::AllLevels;

        std::pair<int, std::string> highest = std::make_pair(-1, "");
        for (std::vector<ESM::LevelledListBase::LevelItem>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (playerLevel >= it->mLevel
                    && (allLevels || it->mLevel == highestLevel))
            {
                candidates.push_back(it->mId);
                if (it->mLevel >= highest.first)
                    highest = std::make_pair(it->mLevel, it->mId);
            }
        }
        if (candidates.empty())
            return std::string();
        std::string item = candidates[Misc::Rng::rollDice(candidates.size())];

        // Vanilla doesn't fail on nonexistent items in levelled lists
        if (!MWBase::Environment::get().getWorld()->getStore().find(Misc::StringUtils::lowerCase(item)))
        {
            std::cerr << "Warning: ignoring nonexistent item '" << item << "' in levelled list '" << levItem->mId << "'" << std::endl;
            return std::string();
        }

        // Is this another levelled item or a real item?
        MWWorld::ManualRef ref (MWBase::Environment::get().getWorld()->getStore(), item, 1);
        if (ref.getPtr().getTypeName() != typeid(ESM::ItemLevList).name()
                && ref.getPtr().getTypeName() != typeid(ESM::CreatureLevList).name())
        {
            return item;
        }
        else
        {
            if (ref.getPtr().getTypeName() == typeid(ESM::ItemLevList).name())
                return getLevelledItem(ref.getPtr().get<ESM::ItemLevList>()->mBase, false, failChance);
            else
                return getLevelledItem(ref.getPtr().get<ESM::CreatureLevList>()->mBase, true, failChance);
        }
    }

    inline std::string getLeveledObject(const std::vector<ESM4::LVLO>& items, bool allLevels, uint8_t failChance)
    {
        const MWWorld::Ptr& player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        int playerLevel = player.getClass().getCreatureStats(player).getLevel();

        if (Misc::Rng::roll0to99() < int(failChance)) // TODO: maybe uint8_t wasn't the best choice
            return std::string();

        std::vector<std::string> candidates;
        int highestLevel = 0;
        for (std::vector<ESM4::LVLO>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (it->level > highestLevel && it->level <= playerLevel)
                highestLevel = it->level;
        }

        std::pair<int, std::string> highest = std::make_pair(-1, "");
        for (std::vector<ESM4::LVLO>::const_iterator it = items.begin(); it != items.end(); ++it)
        {
            if (playerLevel >= it->level && (allLevels || it->level == highestLevel))
            {
                candidates.push_back(ESM4::formIdToString(it->item));
                if (it->level >= highest.first)
                    highest = std::make_pair(it->level, ESM4::formIdToString(it->item));
#if 0
                // FIXME
                MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), ESM4::formIdToString(it->item), 1);
                if (ref.getPtr().getTypeName() == typeid(ESM4::LeveledItem).name()) // only print LVLI
                    std::cout << "candidate " << ref.getPtr().get<ESM4::LeveledItem>()->mBase->mEditorId << std::endl;
#endif
            }
        }

        if (candidates.empty())
            return std::string();

        return candidates[Misc::Rng::rollDice(candidates.size())];
    }

    inline void getTES4LevelledItem(std::vector<std::string>& ids, const ESM4::LeveledItem* levItem, uint8_t failChance = 0)
    {
        const std::vector<ESM4::LVLO>& items = levItem->mLvlObject;
        bool allLevels = levItem->calcAllLvlLessThanPlayer();
        failChance += levItem->chanceNone();
        bool useAll = levItem->useAll();
        if (useAll)
        {
            for (std::size_t i = 0; i < items.size(); ++i)
            {
                std::string itemId = ESM4::formIdToString(items[i].item);
                MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), itemId, 1);
                if (ref.getPtr().getTypeName() == typeid(ESM4::LeveledItem).name())
                {
                    getTES4LevelledItem(ids, ref.getPtr().get<ESM4::LeveledItem>()->mBase, failChance);
                }
                else
                    ids.push_back(itemId);
            }
        }
        else
        {
            std::string itemId = getLeveledObject(items, failChance, allLevels);
            if (itemId.empty())
                return;

            // copy Morrowind behaviour for now
            if (!MWBase::Environment::get().getWorld()->getStore().find(ESM4::stringToFormId(itemId)))
            {
                std::cerr << "Warning: ignoring nonexistent item '" << itemId << "' in levelled list '" <<
                    ESM4::formIdToString(levItem->mFormId) << "'" << std::endl;

                return;
            }

            MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), itemId, 1);

            if (ref.getPtr().getTypeName() == typeid(ESM4::LeveledItem).name())
            {
#if 0
                std::cout << ref.getPtr().get<ESM4::LeveledItem>()->mBase->mEditorId << std::endl; // FIXME:
#endif
                getTES4LevelledItem(ids, ref.getPtr().get<ESM4::LeveledItem>()->mBase, failChance);
            }
            else
                ids.push_back(itemId);
        }
    }

    inline std::string getTES4LevelledCreature(const ESM4::LeveledCreature* levCreature, uint8_t failChance = 0)
    {
        const std::vector<ESM4::LVLO>& items = levCreature->mLvlObject;
        bool allLevels = levCreature->calcAllLvlLessThanPlayer();
        failChance += levCreature->chanceNone();

        std::string item = getLeveledObject(items, failChance, allLevels);

        if (item.empty())
            return std::string();

        // copy Morrowind behaviour for now
        if (!MWBase::Environment::get().getWorld()->getStore().find(ESM4::stringToFormId(item)))
        {
            std::cerr << "Warning: ignoring nonexistent item '" << item << "' in levelled list '" <<
                ESM4::formIdToString(levCreature->mFormId) << "'" << std::endl;

            return std::string();
        }

        MWWorld::ManualRef ref(MWBase::Environment::get().getWorld()->getStore(), item, 1);

        if (ref.getPtr().getTypeName() != typeid(ESM4::LeveledCreature).name())
        {
            return item;
        }
        else
        {
            if (ref.getPtr().getTypeName() == typeid(ESM4::LeveledCreature).name())
            {
                return getTES4LevelledCreature(ref.getPtr().get<ESM4::LeveledCreature>()->mBase, failChance);
            }
            else
                return std::string();
        }
    }
}

#endif
