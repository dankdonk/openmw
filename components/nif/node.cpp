#include "node.hpp"

#include "collision.hpp"
#include "property.hpp"
#include "controller.hpp"
#include "controlled.hpp"
//#include "extra.hpp"

void Nif::Node::read(NIFStream *nif)
{
    Named::read(nif);

    flags = nif->getUShort();
    if (nifVer >= 0x14020007 && (userVer >= 11 || userVer2 > 26)) // from 20.2.0.7
        nif->getUShort();

    trafo = nif->getTrafo(); // scale (float) included

#if 0
    int y = Ogre::Math::ATan2(-trafo.rotation[2][0], trafo.rotation[0][0]).valueDegrees();
    if (/*recIndex == 0 &&*/ y != 0)
        std::cout << this->name << " Y " << y << std::endl;
    int r = Ogre::Math::ATan2(-trafo.rotation[1][2], trafo.rotation[1][1]).valueDegrees();
    if (/*recIndex == 0 &&*/ r != 0)
        std::cout << this->name << " R " << r << std::endl;
    if (name == "ANCastleWallCurve02" || name == "AnCastleWallEnd01")
    {
        std::cout << trafo.rotation[0][0] << ", " << trafo.rotation[0][1] << ", " << trafo.rotation[0][2] << std::endl;
        std::cout << trafo.rotation[1][0] << ", " << trafo.rotation[1][1] << ", " << trafo.rotation[1][2] << std::endl;
        std::cout << trafo.rotation[2][0] << ", " << trafo.rotation[2][1] << ", " << trafo.rotation[2][2] << std::endl;
        if (recIndex == 0)
        {
            std::cout << "root" << std::endl;
            std::cout << "Y " << Ogre::Math::ATan2(-trafo.rotation[2][0], trafo.rotation[0][0]) << std::endl;
            std::cout << "R " << Ogre::Math::ATan2(-trafo.rotation[1][2], trafo.rotation[1][1]) << std::endl;
        }
    }
#endif

    if (nifVer <= 0x04020200) // up to 4.2.2.0
        velocity = nif->getVector3();

    if (nifVer < 0x14020007 || userVer <= 11) // less than 20.2.0.7 (or user version <= 11)
        props.read(nif);

    if (nifVer <= 0x04020200) // up to 4.2.2.0
    {
        hasBounds = nif->getBool(nifVer);
        if(hasBounds)
        {
            nif->getInt(); // always 1
            boundPos = nif->getVector3();
            boundRot = nif->getMatrix3();
            boundXYZ = nif->getVector3();
        }
    }

    parent = NULL;

    boneTrafo = NULL;
    boneIndex = -1;

    if (nifVer >= 0x0a000100) // from 10.0.1.0
        collision.read(nif);
}

void Nif::Node::post(NIFFile *nif)
{
    Named::post(nif);

    if (nifVer < 0x14020007 || userVer <= 11)
        props.post(nif);

    if (nifVer >= 0x0a000100)
        collision.post(nif);
}

void Nif::Node::makeRootBone(const NiSkinData::BoneTrafo *tr)
{
    boneTrafo = tr;
    boneIndex = -1;
}

void Nif::Node::makeBone(short ind, const NiSkinData::BoneInfo &bi)
{
    boneInfo = &bi;
    boneTrafo = &bi.trafo;
    boneIndex = ind;
}

void Nif::Node::getProperties(const Nif::NiTexturingProperty *&texprop,
                              const Nif::NiMaterialProperty *&matprop,
                              const Nif::NiAlphaProperty *&alphaprop,
                              const Nif::NiVertexColorProperty *&vertprop,
                              const Nif::NiZBufferProperty *&zprop,
                              const Nif::NiSpecularProperty *&specprop,
                              const Nif::NiWireframeProperty *&wireprop,
                              const Nif::NiStencilProperty *&stencilprop) const
{
    if(parent)
        parent->getProperties(texprop, matprop, alphaprop, vertprop, zprop, specprop, wireprop, stencilprop);

    for(size_t i = 0; i < props.length(); ++i)
    {
        // Entries may be empty
        if(props[i].empty())
            continue;

        const Nif::Property *pr = props[i].getPtr();

        if(pr->recType == Nif::RC_NiTexturingProperty)
            texprop = static_cast<const Nif::NiTexturingProperty*>(pr);
        else if(pr->recType == Nif::RC_NiMaterialProperty)
            matprop = static_cast<const Nif::NiMaterialProperty*>(pr);
        else if(pr->recType == Nif::RC_NiAlphaProperty)
            alphaprop = static_cast<const Nif::NiAlphaProperty*>(pr);
        else if(pr->recType == Nif::RC_NiVertexColorProperty)
            vertprop = static_cast<const Nif::NiVertexColorProperty*>(pr);
        else if(pr->recType == Nif::RC_NiZBufferProperty)
            zprop = static_cast<const Nif::NiZBufferProperty*>(pr);
        else if(pr->recType == Nif::RC_NiSpecularProperty)
            specprop = static_cast<const Nif::NiSpecularProperty*>(pr);
        else if(pr->recType == Nif::RC_NiWireframeProperty)
            wireprop = static_cast<const Nif::NiWireframeProperty*>(pr);
        else if (pr->recType == Nif::RC_NiStencilProperty)
            stencilprop = static_cast<const Nif::NiStencilProperty*>(pr);
        // the following are unused by the MW engine
        else if (pr->recType != Nif::RC_NiFogProperty
                 && pr->recType != Nif::RC_NiDitherProperty
                 && pr->recType != Nif::RC_NiShadeProperty)
            std::cerr<< "Unhandled property type: "<<pr->recName <<std::endl;
    }
}

void Nif::NodeGroup::read(NIFStream *nif, unsigned int nifVer)
{
    unsigned int numNodes = nif->getUInt();
    nodes.resize(numNodes);
    for (unsigned int i = 0; i < numNodes; ++i)
        nodes[i].read(nif);
}

void Nif::NodeGroup::post(NIFFile *nif)
{
    for (unsigned int i = 0; i < nodes.size(); ++i)
        nodes[i].post(nif);
}

void Nif::NiGeometry::getBSProperties(const Nif::BSLightingShaderProperty *&bsprop,
                                      const Nif::NiAlphaProperty *&alphaprop,
                                      const Nif::BSEffectShaderProperty *&effectprop,
                                      const Nif::BSWaterShaderProperty *&waterprop) const
{
    for(size_t i = 0; i < 2; ++i)
    {
        const Nif::Property *pr = bsprops[i].getPtr();
        if (!pr)
            continue; // may be empty

        if (pr->recType == Nif::RC_BSLightingShaderProperty)
            bsprop = static_cast<const Nif::BSLightingShaderProperty*>(pr);
        else if (pr->recType == Nif::RC_NiAlphaProperty)
            alphaprop = static_cast<const Nif::NiAlphaProperty*>(pr);
        else if (pr->recType == Nif::RC_BSEffectShaderProperty)
            effectprop = static_cast<const Nif::BSEffectShaderProperty*>(pr);
        else if (pr->recType == Nif::RC_BSWaterShaderProperty)
            waterprop = static_cast<const Nif::BSWaterShaderProperty*>(pr);
        else
            std::cout<< "Unhandled property type: "<< pr->recName << std::endl;
    }
}

Ogre::Matrix4 Nif::Node::getLocalTransform() const
{
    Ogre::Matrix4 mat4 = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
    mat4.makeTransform(trafo.pos, Ogre::Vector3(trafo.scale), Ogre::Quaternion(trafo.rotation));
    return mat4;
}

Ogre::Matrix4 Nif::Node::getWorldTransform() const
{
    if(parent != NULL)
        return parent->getWorldTransform() * getLocalTransform();
    return getLocalTransform();
}

void Nif::NiNode::read(NIFStream *nif)
{
    Node::read(nif);
    children.read(nif);
    effects.read(nif);

    // Discard tranformations for the root node, otherwise some meshes
    // occasionally get wrong orientation. Only for NiNode-s for now, but
    // can be expanded if needed.
    if (0 == recIndex && nifVer <= 0x04010000) // FIXME experiment
    {
        if (static_cast<Nif::Node*>(this)->trafo.rotation != Nif::Transformation::getIdentity().rotation)
            std::cout << "Non-identity rotation: " << this->name << ", ver " << std::hex << nifVer << std::endl;
        static_cast<Nif::Node*>(this)->trafo = Nif::Transformation::getIdentity();
    }
}

void Nif::NiNode::post(NIFFile *nif)
{
    Node::post(nif);
    children.post(nif);
    effects.post(nif);

    for(size_t i = 0;i < children.length();i++)
    {
        // Why would a unique list of children contain empty refs?
        if(!children[i].empty())
            children[i]->parent = this;
    }
}

void Nif::BSMultiBound::read(NIFStream *nif)
{
    data.read(nif);
}

void Nif::BSMultiBound::post(NIFFile *nif)
{
    data.post(nif);
}

void Nif::BSMultiBoundOBB::read(NIFStream *nif)
{
    center = nif->getVector3();
    size = nif->getVector3();
    rotation = nif->getMatrix3();
}

void Nif::BSTreeNode::read(NIFStream *nif)
{
    NiNode::read(nif);

    unsigned int numBones = nif->getUInt();
    bones1.resize(numBones);
    for (unsigned int i = 0; i < numBones; ++i)
        bones1[i].read(nif);

    numBones = nif->getUInt();
    bones2.resize(numBones);
    for (unsigned int i = 0; i < numBones; ++i)
        bones2[i].read(nif);
}

void Nif::BSTreeNode::post(NIFFile *nif)
{
    NiNode::post(nif);

    for (unsigned int i = 0; i < bones1.size(); ++i)
        bones1[i].post(nif);

    for (unsigned int i = 0; i < bones2.size(); ++i)
        bones2[i].post(nif);
}

void Nif::BSValueNode::read(NIFStream *nif)
{
    NiNode::read(nif);

    value = nif->getInt();

    nif->getChar(); // unknown byte
}

void Nif::BSValueNode::post(NIFFile *nif)
{
    NiNode::post(nif);
}

void Nif::BSOrderedNode::read(NIFStream *nif)
{
    NiNode::read(nif);

    alphaSortBound = nif->getVector4();
    isStaticBound = nif->getChar();
}

void Nif::BSOrderedNode::post(NIFFile *nif)
{
    NiNode::post(nif);
}

void Nif::BSMultiBoundNode::read(NIFStream *nif)
{
    NiNode::read(nif);

    multiBound.read(nif);
    if (nifVer >= 0x14020007) // from 20.2.0.7
        unknown = nif->getUInt();
}

void Nif::BSMultiBoundNode::post(NIFFile *nif)
{
    NiNode::post(nif);

    multiBound.post(nif);
}

void Nif::BSBlastNode::read(NIFStream *nif)
{
    NiNode::read(nif);

    unknown1 = nif->getChar();
    unknown2 = nif->getShort();
}

void Nif::BSBlastNode::post(NIFFile *nif)
{
    NiNode::post(nif);
}

void Nif::NiSwitchNode::read(NIFStream *nif)
{
    NiNode::read(nif);

    if (nifVer >= 0x0a010000) // from 10.1.0.0
        nif->getUShort();

    nif->getInt();
}

void Nif::NiSwitchNode::post(NIFFile *nif)
{
    NiNode::post(nif);
}

void Nif::BSDamageStage::read(NIFStream *nif)
{
    NiNode::read(nif);

    unknown1 = nif->getChar();
    unknown2 = nif->getShort();
}

void Nif::BSDamageStage::post(NIFFile *nif)
{
    NiNode::post(nif);
}

void Nif::NiBillboardNode::read(NIFStream *nif)
{
    NiNode::read(nif);

    if (nifVer >= 0x0a010000) // from 10.1.0.0
        billboardMode = nif->getUShort();
}

void Nif::NiBillboardNode::post(NIFFile *nif)
{
    NiNode::post(nif);
}

void Nif::NiCamera::Camera::read(NIFStream *nif, unsigned int nifVer)
{
    if (nifVer >= 0x0a010000) // from 10.1.0.0
        nif->getUShort();
    left = nif->getFloat();
    right = nif->getFloat();
    top = nif->getFloat();
    bottom = nif->getFloat();
    nearDist = nif->getFloat();
    farDist = nif->getFloat();
    if (nifVer >= 0x0a010000) // from 10.1.0.0
        useOrtho = nif->getBool(nifVer);

    vleft = nif->getFloat();
    vright = nif->getFloat();
    vtop = nif->getFloat();
    vbottom = nif->getFloat();

    LOD = nif->getFloat();
}

void Nif::NiCamera::read(NIFStream *nif)
{
    Node::read(nif);

    cam.read(nif, nifVer);

    nif->getInt(); // -1
    nif->getInt(); // 0
    if (nifVer >= 0x04020100) // from 4.2.1.0
        nif->getInt();
}

void Nif::NiCamera::post(NIFFile *nif)
{
    Node::post(nif);
}

void Nif::NiGeometry::read(NIFStream *nif)
{
    Node::read(nif); // NiAVObject
    data.read(nif);  // refNiGeometryData
    skin.read(nif);

    if (nifVer >= 0x14020007) // from 20.2.0.7 (Skyrim)
    {
        unsigned int numMaterials = nif->getUInt();
        materials.resize(numMaterials);
        for (unsigned int i = 0; i < numMaterials; ++i)
        {
            materials[i].name = nif->getString();
            materials[i].extraData = nif->getInt();
        }
        nif->getInt(); // active material?
    }

    if (nifVer >= 0x0a000100 && nifVer <= 0x14010003 && nif->getBool(nifVer))
    {
        shader = nif->getString();
        unknown = nif->getInt();
    }

    if (nifVer >= 0x14020007) // from 20.2.0.7 (Skyrim)
    {
        dirtyFlag = nif->getBool(nifVer);
        bsprops.resize(2);
        bsprops[0].read(nif);
        bsprops[1].read(nif);
    }
}

void Nif::NiGeometry::post(NIFFile *nif)
{
    Node::post(nif);
    data.post(nif);
    skin.post(nif);
    if (nifVer >= 0x14020007) // from 20.2.0.7 (Skyrim)
    {
        bsprops[0].post(nif);
        bsprops[1].post(nif);
    }
}

void Nif::NiParticleSystem::read(NIFStream *nif)
{
    NiGeometry::read(nif);

    if (userVer >= 12)
    {
        unknownS2 = nif->getUShort();
        unknownS3 = nif->getUShort();
        unknownI1 = nif->getInt();
    }

    if (nifVer >= 0x0a010000) // from 10.1.0.0
    {
        worldSpace = nif->getBool(nifVer);
        unsigned int numModifiers = nif->getUInt();
        modifiers.resize(numModifiers);
        for (unsigned int i = 0; i < numModifiers; ++i)
            modifiers[i].read(nif);
    }
}

void Nif::NiParticleSystem::post(NIFFile *nif)
{
    NiGeometry::post(nif);

    if (nifVer >= 0x0a010000) // from 10.1.0.0
        for (unsigned int i = 0; i < modifiers.size(); ++i)
            modifiers[i].post(nif);
}

void Nif::BSLODTriShape::read(NIFStream *nif)
{
    NiGeometry::read(nif);

    level0Size = nif->getUInt();
    level1Size = nif->getUInt();
    level2Size = nif->getUInt();
}

void Nif::BSLODTriShape::post(NIFFile *nif)
{
    NiGeometry::post(nif);
}
