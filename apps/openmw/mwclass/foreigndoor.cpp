#include "foreigndoor.hpp"

#include <extern/esm4/cell.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/actionteleportforeign.hpp"
#include "../mwworld/foreigncell.hpp"
#include "../mwworld/foreignworld.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace MWClass
{
    std::string ForeignDoor::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Door>()->mBase->mEditorId;
    }

    // FIXME: having the dummy cell results in *all* the doors being rendered!  Need to be
    // able to limit rendering based on the ref's position
    void ForeignDoor::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref = ptr.get<ESM4::Door>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignDoor::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignDoor::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref = ptr.get<ESM4::Door>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    // This is required for activations to work
    std::string ForeignDoor::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref =
            ptr.get<ESM4::Door>();

        return ref->mBase->mFullName;
    }

    void ForeignDoor::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignDoor);

        registerClass (typeid (ESM4::Door).name(), instance);
    }

    MWWorld::Ptr ForeignDoor::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref = ptr.get<ESM4::Door>();

        return MWWorld::Ptr(&cell.get<ESM4::Door>().insert(*ref), &cell);
    }

    boost::shared_ptr<MWWorld::Action> ForeignDoor::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref = ptr.get<ESM4::Door>();
#if 0
        const std::string &openSound = ref->mBase->mOpenSound;
        const std::string &closeSound = ref->mBase->mCloseSound;
        const std::string lockedSound = "LockedDoor";
        const std::string trapActivationSound = "Disarm Trap Fail";

        MWWorld::ContainerStore &invStore = actor.getClass().getContainerStore(actor);

        bool needKey = ptr.getCellRef().getLockLevel() > 0;
        bool hasKey = false;
        std::string keyName;

        // make key id lowercase
        std::string keyId = ptr.getCellRef().getKey();
        Misc::StringUtils::lowerCaseInPlace(keyId);
        for (MWWorld::ContainerStoreIterator it = invStore.begin(); it != invStore.end(); ++it)
        {
            std::string refId = it->getCellRef().getRefId();
            Misc::StringUtils::lowerCaseInPlace(refId);
            if (refId == keyId)
            {
                hasKey = true;
                keyName = it->getClass().getName(*it);
            }
        }

        if (needKey && hasKey)
        {
            if(actor == MWBase::Environment::get().getWorld()->getPlayerPtr())
                MWBase::Environment::get().getWindowManager()->messageBox(keyName + " #{sKeyUsed}");
            unlock(ptr); //Call the function here. because that makes sense.
            // using a key disarms the trap
            ptr.getCellRef().setTrap("");
        }

        if (!needKey || hasKey)
#endif
        if(1)
        {
#if 0
            if(!ptr.getCellRef().getTrap().empty())
            {
                // Trap activation
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTrap(actor, ptr.getCellRef().getTrap(), ptr));
                action->setSound(trapActivationSound);
                return action;
            }
#endif
            if (ptr.getCellRef().getTeleport())
            {
                std::cout << "open door" << std::endl;
                const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
                const ESM4::FormId cellId = store.getDoorCell(ptr.getCellRef().getDestDoor());
                const MWWorld::ForeignCell *cell = store.get<MWWorld::ForeignCell>().find(cellId);
                // FIXME check cell is not null
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionTeleportForeign(
                            cell->mCell->mEditorId,
                            cellId,
                            ptr.getCellRef().getDoorDest(), true));

                //action->setSound(openSound); // FIXME

                return action;
            }
#if 0
            else
            {
                // animated door
                boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionDoor(ptr));
                int doorstate = getDoorState(ptr);
                bool opening = true;
                if (doorstate == 1)
                    opening = false;
                if (doorstate == 0 && ptr.getRefData().getLocalRotation().rot[2] != 0)
                    opening = false;

                if (opening)
                {
                    MWBase::Environment::get().getSoundManager()->fadeOutSound3D(ptr,
                            closeSound, 0.5f);
                    float offset = ptr.getRefData().getLocalRotation().rot[2]/ 3.14159265f * 2.0f;
                    action->setSoundOffset(offset);
                    action->setSound(openSound);
                }
                else
                {
                    MWBase::Environment::get().getSoundManager()->fadeOutSound3D(ptr,
                                                openSound, 0.5f);
                    float offset = 1.0f - ptr.getRefData().getLocalRotation().rot[2]/ 3.14159265f * 2.0f;
                    //most if not all door have closing bang somewhere in the middle of the sound,
                    //so we divide offset by two
                    action->setSoundOffset(offset * 0.5f);
                    action->setSound(closeSound);
                }

                return action;
            }
#endif
        }
#if 0
        else
        {
            // locked, and we can't open.
            boost::shared_ptr<MWWorld::Action> action(new MWWorld::FailedAction);
            action->setSound(lockedSound);
            return action;
        }
#endif
    }

    bool ForeignDoor::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref =
            ptr.get<ESM4::Door>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignDoor::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref =
            ptr.get<ESM4::Door>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName;

        std::string text;

        if (ptr.getCellRef().getTeleport())
        {
            text += "\n#{sTo}";
            text += "\n" + getDestination(*ref);
        }

        if (ptr.getCellRef().getLockLevel() > 0)
            text += "\n#{sLockLevel}: " + MWGui::ToolTips::toString(ptr.getCellRef().getLockLevel());
        else if (ptr.getCellRef().getLockLevel() < 0)
            text += "\n#{sUnlocked}";
        if (ptr.getCellRef().getTrap() != "")
            text += "\n#{sTrapped}";

        if (MWBase::Environment::get().getWindowManager()->getFullHelp())
        {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }
        info.text = text;

        return info;
    }

    std::string ForeignDoor::getDestination (const MWWorld::LiveCellRef<ESM4::Door>& door)
    {
        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::string dest;
        if (door.mRef.getDestCell() != "")
        {
            // door leads to an interior, use interior name as tooltip
            // (or external cell with full name)
            dest = door.mRef.getDestCell();
        }
        else
        {
            // use world name if exists
            const ESM4::FormId cellId = store.getDoorCell(door.mRef.getDestDoor());
            const MWWorld::ForeignCell *cell = store.get<MWWorld::ForeignCell>().find(cellId);

            if (cell)
            {
                ESM4::FormId worldId = cell->mCell->mParent;
                const MWWorld::ForeignWorld * world = store.get<MWWorld::ForeignWorld>().find(worldId);
                if (!world->mFullName.empty())
                    dest = world->mFullName;
            }
        }

        return "#{sCell=" + dest + "}";
    }
}
