#include "node.hpp"

#include "collision.hpp"
#include "property.hpp"
#include "controller.hpp"
#include "controlled.hpp"
//#include "extra.hpp"

void Nif::Node::getProperties(const Nif::NiTexturingProperty *&texprop,
                              const Nif::NiMaterialProperty *&matprop,
                              const Nif::NiAlphaProperty *&alphaprop,
                              const Nif::NiVertexColorProperty *&vertprop,
                              const Nif::NiZBufferProperty *&zprop,
                              const Nif::NiSpecularProperty *&specprop,
                              const Nif::NiWireframeProperty *&wireprop,
                              const Nif::NiStencilProperty *&stencilprop,
                              const Nif::Property *&prop) const
{
    if(parent)
        parent->getProperties(texprop, matprop, alphaprop, vertprop, zprop, specprop, wireprop, stencilprop, prop);

    for(size_t i = 0;i < props.length();++i)
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
        else if (pr->recType == Nif::RC_BSShaderPPLightingProperty ||
                 pr->recType == Nif::RC_BSShaderNoLightingProperty)
        {
            prop = pr; // FO3 handled elsewhere
        }
        // the following are unused by the MW engine
        else if (pr->recType != Nif::RC_NiFogProperty
                 && pr->recType != Nif::RC_NiDitherProperty
                 && pr->recType != Nif::RC_NiShadeProperty)
            std::cerr<< "Unhandled property type: "<<pr->recName <<std::endl;
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
            std::cout<< "Unhandled BS property type: "<< pr->recName << std::endl;
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

void Nif::BSFadeNode::read(NIFStream *nif)
{
    NiNode::read(nif);

    // Discard tranformations for the root node, otherwise some meshes
    // occasionally get wrong orientation. Only for NiNode-s for now, but
    // can be expanded if needed.
    if (0 == recIndex/* && nifVer <= 0x04010000*/) // FIXME experiment
    {
        if (static_cast<Nif::Node*>(this)->trafo.rotation != Nif::Transformation::getIdentity().rotation)
            std::cout << "Non-identity rotation: " << this->name << ", ver " << std::hex << nifVer << std::endl;
        static_cast<Nif::Node*>(this)->trafo = Nif::Transformation::getIdentity();
    }
}

void Nif::BSFadeNode::post(NIFFile *nif)
{
    NiNode::post(nif);
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
