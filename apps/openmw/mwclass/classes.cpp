#include "classes.hpp"

#include "activator.hpp"
#include "creature.hpp"
#include "npc.hpp"
#include "weapon.hpp"
#include "armor.hpp"
#include "potion.hpp"
#include "apparatus.hpp"
#include "book.hpp"
#include "clothing.hpp"
#include "container.hpp"
#include "door.hpp"
#include "ingredient.hpp"
#include "creaturelevlist.hpp"
#include "itemlevlist.hpp"
#include "light.hpp"
#include "lockpick.hpp"
#include "misc.hpp"
#include "probe.hpp"
#include "repair.hpp"
#include "static.hpp"

#include "foreignsound.hpp"
#include "foreignactivator.hpp"
#include "foreignapparatus.hpp"
#include "foreignarmor.hpp"
#include "foreignbook.hpp"
#include "foreignclothing.hpp"
#include "foreigncontainer.hpp"
#include "foreigndoor.hpp"
#include "foreigningredient.hpp"
#include "foreignlight.hpp"
#include "foreignmiscitem.hpp"
#include "foreignstatic.hpp"
#include "foreigngrass.hpp"
#include "foreigntree.hpp"
#include "foreignflora.hpp"
#include "foreignfurniture.hpp"
#include "foreignweapon.hpp"
#include "foreignammo.hpp"
#include "foreignnpc.hpp"
#include "foreigncreature.hpp"
#include "foreignleveledcreature.hpp"
#include "foreignsoulgem.hpp"
#include "foreignkey.hpp"
#include "foreignpotion.hpp"
#include "foreignsigilstone.hpp"
#include "foreignleveleditem.hpp"

namespace MWClass
{
    void registerClasses()
    {
        Activator::registerSelf();
        Creature::registerSelf();
        Npc::registerSelf();
        Weapon::registerSelf();
        Armor::registerSelf();
        Potion::registerSelf();
        Apparatus::registerSelf();
        Book::registerSelf();
        Clothing::registerSelf();
        Container::registerSelf();
        Door::registerSelf();
        Ingredient::registerSelf();
        CreatureLevList::registerSelf();
        ItemLevList::registerSelf();
        Light::registerSelf();
        Lockpick::registerSelf();
        Miscellaneous::registerSelf();
        Probe::registerSelf();
        Repair::registerSelf();
        Static::registerSelf();

        ForeignSound::registerSelf();
        ForeignActivator::registerSelf();
        ForeignApparatus::registerSelf();
        ForeignArmor::registerSelf();
        ForeignBook::registerSelf();
        ForeignClothing::registerSelf();
        ForeignContainer::registerSelf();
        ForeignDoor::registerSelf();
        ForeignIngredient::registerSelf();
        ForeignLight::registerSelf();
        ForeignMiscItem::registerSelf();
        ForeignStatic::registerSelf();
        ForeignGrass::registerSelf();
        ForeignTree::registerSelf();
        ForeignFlora::registerSelf();
        ForeignFurniture::registerSelf();
        ForeignWeapon::registerSelf();
        ForeignAmmo::registerSelf();
        ForeignNpc::registerSelf();
        ForeignCreature::registerSelf();
        ForeignLeveledCreature::registerSelf();
        ForeignSoulGem::registerSelf();
        ForeignKey::registerSelf();
        ForeignPotion::registerSelf();
        ForeignSigilStone::registerSelf();
        ForeignLeveledItem::registerSelf();
    }
}
