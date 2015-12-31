#ifndef COMPONENTS_NIFOGRE_MESH_HPP
#define COMPONENTS_NIFOGRE_MESH_HPP

#include <iostream>
#include <string>
#include <map>
#include <cassert>

#include <OgreResource.h>

namespace Nif
{
    struct Record;
    //struct NiTriShape;
    //struct NiTriStrips;
}

namespace NifOgre
{

/** Manual resource loader for NiTriShapes. This is the main class responsible
 * for translating the internal NIF meshes into something Ogre can use.
 */
class NIFMeshLoader : Ogre::ManualResourceLoader
{
    static void warn(const std::string &msg)
    {
        std::cerr << "NIFMeshLoader: Warn: " << msg << std::endl;
    }

    static void fail(const std::string &msg)
    {
        std::cerr << "NIFMeshLoader: Fail: "<< msg << std::endl;
        abort();
    }

    std::string mName;
    std::string mGroup;
    size_t mShapeIndex;
    bool mStrips;

    // Convert NiTriShape to Ogre::SubMesh
    //void createSubMesh(Ogre::Mesh *mesh, const Nif::NiTriShape *shape);
    void createSubMesh(Ogre::Mesh *mesh, const Nif::Record *record);
    //void createStripsSubMesh(Ogre::Mesh *mesh, const Nif::NiTriStrips *strips);

    typedef std::map<std::string,NIFMeshLoader> LoaderMap;
    static LoaderMap sLoaders;

    NIFMeshLoader(const std::string &name, const std::string &group, size_t idx, bool strips);

    virtual void loadResource(Ogre::Resource *resource);

public:
    static void createMesh(const std::string &name, const std::string &fullname, const std::string &group, size_t idx, bool strips);
};

}

#endif
