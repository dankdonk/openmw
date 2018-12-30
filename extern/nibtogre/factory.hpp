/*
  Copyright (C) 2017-2018 cc9cii

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

  The factory pattern is based on the answer provided by Roger Pate in
  https://stackoverflow.com/questions/1832003/instantiating-classes-by-name-with-factory-pattern

*/
#ifndef NIBTOGRE_FACTORY_H
#define NIBTOGRE_FACTORY_H

#include <memory>
#include <map>
#include <string>
#include <stdexcept>

#include <boost/current_function.hpp>

namespace NiBtOgre
{
    class NiModel;
    class NiStream;
    struct ModelData;

    template<class Interface, class KeyT=std::string>
    class Factory
    {
    public:
        typedef KeyT Key;
        typedef std::unique_ptr<Interface> Type;
        typedef Type (*Creator)(uint32_t, NiStream&, const NiModel&, ModelData&);
        typedef std::map<Key, Creator> Registry;

        // Define key -> v relationship, return whether this is a new key.
        bool define(Key const& key, Creator v)
        {
            return mRegistry.insert(typename Registry::value_type(key, v)).second;
        }

        Type create(Key const& key,
                uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
        {
            typename Registry::const_iterator iter = mRegistry.find(key);

            if (iter == mRegistry.end())
                throw std::invalid_argument(std::string(BOOST_CURRENT_FUNCTION) + ": key not registered");
            else
                return iter->second(index, stream, model, data);
        }

        template<class Base, class Derived>
        static std::unique_ptr<Base> create_func(
                uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
        {
            return std::unique_ptr<Base>(new Derived(index, stream, model, data));
        }

    private:
        Registry mRegistry;
    };
}

#endif // NIBTOGRE_FACTORY_H
