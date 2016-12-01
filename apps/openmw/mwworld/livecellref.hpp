#ifndef GAME_MWWORLD_LIVECELLREF_H
#define GAME_MWWORLD_LIVECELLREF_H

#include <typeinfo>

#include <extern/esm4/refr.hpp>
#include <extern/esm4/acre.hpp>
#include <extern/esm4/achr.hpp>

#include "cellref.hpp"

#include "refdata.hpp"

namespace ESM
{
    struct ObjectState;
}

namespace MWWorld
{
    class Ptr;
    class ESMStore;
    class Class;

    /// Used to create pointers to hold any type of LiveCellRef<> object.
    struct LiveCellRefBase
    {
        const Class *mClass;

        /** Information about this instance, such as 3D location and rotation
         * and individual type-dependent data.
         */
        MWWorld::CellRef mRef;

        /** runtime-data */
        RefData mData;

        LiveCellRefBase(std::string type, const ESM::CellRef &cref=ESM::CellRef());

        // FIXME: having 3 separate constructor is rather poor form...
        LiveCellRefBase(std::string type, const ESM4::Reference& cref);
        LiveCellRefBase(std::string type, const ESM4::ActorCreature& cref);
        LiveCellRefBase(std::string type, const ESM4::ActorCharacter& cref);

        /* Need this for the class to be recognized as polymorphic */
        virtual ~LiveCellRefBase() { }

        virtual void load (const ESM::ObjectState& state) = 0;
        ///< Load state into a LiveCellRef, that has already been initialised with base and class.
        ///
        /// \attention Must not be called with an invalid \a state.

        virtual void save (ESM::ObjectState& state) const = 0;
        ///< Save LiveCellRef state into \a state.

        protected:

            void loadImp (const ESM::ObjectState& state);
            ///< Load state into a LiveCellRef, that has already been initialised with base and
            /// class.
            ///
            /// \attention Must not be called with an invalid \a state.

            void saveImp (ESM::ObjectState& state) const;
            ///< Save LiveCellRef state into \a state.

            static bool checkStateImp (const ESM::ObjectState& state);
            ///< Check if state is valid and report errors.
            ///
            /// \return Valid?
            ///
            /// \note Does not check if the RefId exists.
    };

    inline bool operator== (const LiveCellRefBase& cellRef, const ESM::RefNum refNum)
    {
        return cellRef.mRef.getRefNum()==refNum;
    }

    // Can't use ESM4::FormId as the second parameter because the compiler gets confused.
    inline bool operator== (const LiveCellRefBase& cellRef, const ESM4::Reference& ref)
    {
        return cellRef.mRef.getFormId() == ref.mFormId;
    }

    inline bool operator== (const LiveCellRefBase& cellRef, const ESM4::ActorCreature& ref)
    {
        return cellRef.mRef.getFormId() == ref.mFormId;
    }

    inline bool operator== (const LiveCellRefBase& cellRef, const ESM4::ActorCharacter& ref)
    {
        return cellRef.mRef.getFormId() == ref.mFormId;
    }

    /// A reference to one object (of any type) in a cell.
    ///
    /// Constructing this with a CellRef instance in the constructor means that
    /// in practice (where D is RefData) the possibly mutable data is copied
    /// across to mData. If later adding data (such as position) to CellRef
    /// this would have to be manually copied across.
    template <typename X>
    struct LiveCellRef : public LiveCellRefBase
    {
        LiveCellRef(const ESM::CellRef& cref, const X* b = NULL)
            : LiveCellRefBase(typeid(X).name(), cref), mBase(b)
        {}

        LiveCellRef(const X* b = NULL)
            : LiveCellRefBase(typeid(X).name()), mBase(b)
        {}

        LiveCellRef(const ESM4::Reference& cref, const X* b = NULL)
            : LiveCellRefBase(typeid(X).name(), cref), mBase(b)
        {}

        LiveCellRef(const ESM4::ActorCreature& cref, const X* b = NULL)
            : LiveCellRefBase(typeid(X).name(), cref), mBase(b)
        {}

        LiveCellRef(const ESM4::ActorCharacter& cref, const X* b = NULL)
            : LiveCellRefBase(typeid(X).name(), cref), mBase(b)
        {}

        // The object that this instance is based on.
        const X* mBase;

        virtual void load (const ESM::ObjectState& state);
        ///< Load state into a LiveCellRef, that has already been initialised with base and class.
        ///
        /// \attention Must not be called with an invalid \a state.

        virtual void save (ESM::ObjectState& state) const;
        ///< Save LiveCellRef state into \a state.

        static bool checkState (const ESM::ObjectState& state);
        ///< Check if state is valid and report errors.
        ///
        /// \return Valid?
        ///
        /// \note Does not check if the RefId exists.
    };

    template <typename X>
    void LiveCellRef<X>::load (const ESM::ObjectState& state)
    {
        loadImp (state);
    }

    template <typename X>
    void LiveCellRef<X>::save (ESM::ObjectState& state) const
    {
        saveImp (state);
    }

    template <typename X>
    bool LiveCellRef<X>::checkState (const ESM::ObjectState& state)
    {
        return checkStateImp (state);
    }

}

#endif
