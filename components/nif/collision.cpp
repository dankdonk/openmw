#include "collision.hpp"

#include "node.hpp"

void Nif::hkTriangle::read(NIFStream *nif, unsigned int nifVer)
{
    triangle.resize(3);
    triangle[0] = nif->getShort();
    triangle[1] = nif->getShort();
    triangle[2] = nif->getShort();
    weldingInfo = nif->getUShort();
    if (nifVer <= 0x14000005) // up to 20.0.0.5 only
        normal = nif->getVector3();
}

void Nif::OblivionSubShape::read(NIFStream *nif)
{
    layer = nif->getChar();
    collisionFilter = nif->getChar();
    unknown = nif->getUShort();
    numVerts = nif->getUInt();
    material = nif->getUInt();
}

void Nif::hkPackedNiTriStripsData::read(NIFStream *nif)
{
    unsigned int numTris = nif->getUInt();
    triangles.resize(numTris);
    for(unsigned int i = 0; i < numTris; i++)
        triangles[i].read(nif, nifVer);

    unsigned int numVerts = nif->getUInt();
    if (nifVer >= 0x14020007) // from 20.2.0.7
        nif->getChar();
    vertices.resize(numVerts);
    for(unsigned int i = 0; i < numVerts; i++)
        vertices[i] = nif->getVector3();

    if (nifVer >= 0x14020007) // from 20.2.0.7
    {
        unsigned short numSubShapes = nif->getUShort();
        subShapes.resize(numSubShapes);
        for(unsigned int i = 0; i < numSubShapes; i++)
            subShapes[i].read(nif);
    }
}

void Nif::bhkPackedNiTriStripsShape::read(NIFStream *nif)
{
    if (nifVer <= 0x14000005) // up to 20.0.0.5 only
    {
        unsigned short numShapes = nif->getUShort();
        subShapes.resize(numShapes);
        for (unsigned int i = 0; i < numShapes; ++i)
            subShapes[i].read(nif);
    }

    unknown1 = nif->getUInt();
    unknown2 = nif->getUInt();
    unknownF1 = nif->getFloat();
    unknown3 = nif->getUInt();
    unknownVec = nif->getVector3();
    unknownF2 = nif->getFloat();
    unknownF3 = nif->getFloat();
    scale = nif->getVector3();
    unknownF4 = nif->getFloat();

    data.read(nif);
}

void Nif::bhkPackedNiTriStripsShape::post(NIFFile *nif)
{
    data.post(nif);
}

void Nif::bhkNiTriStripsShape::read(NIFStream *nif)
{
    material = nif->getUInt();
    unknownF1 = nif->getFloat();
    unknown1 = nif->getUInt();
    unknownInts.resize(4);
    for (unsigned int i = 0; i < 4; ++i)
        unknownInts[i] = nif->getUInt();
    unknown2 = nif->getUInt();

    scale = nif->getVector3();
    unknown3 = nif->getUInt();

    unsigned short numStrips = nif->getUInt();
    stripsData.resize(numStrips);
    for (unsigned int i = 0; i < numStrips; ++i)
    {
        stripsData[i].read(nif);
    }
    unsigned short numDataLayers = nif->getUInt();
    dataLayers.resize(numDataLayers);
    for (unsigned int i = 0; i < numDataLayers; ++i)
    {
        dataLayers[i].layer = nif->getChar();
        dataLayers[i].colFilter = nif->getChar();
        dataLayers[i].unknown = nif->getUShort();
    }
}

void Nif::bhkNiTriStripsShape::post(NIFFile *nif)
{
    for (unsigned int i = 0; i < stripsData.size(); ++i)
        stripsData[i].post(nif);
}

void Nif::bhkListShape::read(NIFStream *nif)
{
    numSubShapes = nif->getUInt();
    subShapes.resize(numSubShapes);
    for (unsigned int i = 0; i < numSubShapes; ++i)
        subShapes[i].read(nif);

    material = nif->getUInt();

    unknown.resize(6);
    for (int i = 0; i < 6; ++i)
        unknown[i] = nif->getFloat();

    unknownInts.resize(nif->getUInt());
    for (unsigned int i = 0; i < unknownInts.size(); ++i)
        unknownInts[i] = nif->getUInt();
}

void Nif::bhkListShape::post(NIFFile *nif)
{
    for (unsigned int i = 0; i < subShapes.size(); ++i)
        subShapes[i].post(nif);
}

void Nif::bhkBoxShape::read(NIFStream *nif)
{
    material = nif->getUInt();
    radius = nif->getFloat();
    unknown.resize(8);
    for (int i = 0; i < 8; ++i)
        unknown[i] = nif->getChar();

    dimensions = nif->getBtVector3();
    dimensions_old = Ogre::Vector3(dimensions.getX(), dimensions.getY(), dimensions.getZ()); // deprecated
    minSize = nif->getFloat();
}

void Nif::bhkSphereShape::read(NIFStream *nif)
{
    material = nif->getUInt();
    radius = nif->getFloat();
}

void Nif::bhkMultiSphereShape::read(NIFStream *nif)
{
    material = nif->getUInt();
    radius = nif->getFloat();
    unknown1 = nif->getFloat();
    unknown2 = nif->getFloat();
    unsigned int numSpheres = nif->getUInt();
    spheres.resize(numSpheres);
    for (unsigned int i = 0; i < numSpheres; ++i)
    {
        spheres[i].center = nif->getVector3();
        spheres[i].radius = nif->getFloat();
    }
}

void Nif::bhkTransformShape::read(NIFStream *nif)
{
    shape.read(nif);
    material = nif->getUInt();
    unknownF1 = nif->getFloat();
    unknown.resize(8);
    for (int i = 0; i < 8; ++i)
        unknown[i] = nif->getChar();
    float floats[16];
    for (int i = 0; i < 16; ++i)
        floats[i] = nif->getFloat();
    transform = Ogre::Matrix4(floats[0], floats[4], floats[8],  floats[12],
                              floats[1], floats[5], floats[9],  floats[13],
                              floats[2], floats[6], floats[10], floats[14],
                              floats[3], floats[7], floats[11], floats[15]);
}

void Nif::bhkTransformShape::post(NIFFile *nif)
{
    shape.post(nif);
}

void Nif::bhkCapsuleShape::read(NIFStream *nif)
{
    material = nif->getUInt();
    radius = nif->getFloat();

    std::vector<char> unknown;
    unknown.resize(8);
    for (int i = 0; i < 8; ++i)
        unknown[i] = nif->getChar();

    firstPoint= nif->getVector3();
    radius1= nif->getFloat();
    secondPoint= nif->getVector3();
    radius2= nif->getFloat();
}

void Nif::bhkConvexVerticesShape::read(NIFStream *nif)
{
    bhkSphereShape::read(nif);

    unknown.resize(6);
    for (int i = 0; i < 6; ++i)
        unknown[i] = nif->getFloat();

    nif->getVector4s(vertices_old, nif->getUInt());
    vertices.resize(vertices_old.size());
    for (unsigned int i = 0; i < vertices_old.size(); ++i) // FIXME deprecated
    {
        vertices[i] = btVector3(vertices_old[i].x, vertices_old[i].y, vertices_old[i].z);
    }
    nif->getVector4s(normals, nif->getUInt());
}

void Nif::bhkConvexVerticesShape::post(NIFFile *nif)
{
    //bhkSphereShape::post(nif);
    // FIXME
}

void Nif::bhkMoppBvTreeShape::read(NIFStream *nif)
{
    shape.read(nif);
    material = nif->getUInt(); //if userVer >= 12, Skyrim material

    unknown.resize(8);
    for (int i = 0; i < 8; ++i)
        unknown[i] = nif->getChar();

    unknownF1 = nif->getFloat();

    unsigned int dataSize = nif->getUInt();
    origin = nif->getVector3();
    scale = nif->getFloat();

    moppData.resize(dataSize);
    for (unsigned int i = 0; i < dataSize; ++i)
        moppData[i] = nif->getChar();

    if (nifVer >= 0x14020005 && userVer >= 12) // from 20.2.0.7
        nif->getChar(); // Unknown Byte1
}

void Nif::bhkMoppBvTreeShape::post(NIFFile *nif)
{
    shape.post(nif);
}

void Nif::bhkCompressedMeshShape::read(NIFStream *nif)
{
    target.read(nif);
    materialSkyrim = nif->getUInt();
    nif->getFloat();
    unknown.resize(4);
    for (int i = 0; i < 4; ++i)
        unknown[i] = nif->getChar();
    nif->getVector4();
    radius = nif->getFloat();
    scale = nif->getFloat();
    nif->getFloat();
    nif->getFloat();
    nif->getFloat();
    data.read(nif);
}

void Nif::bhkCompressedMeshShape::post(NIFFile *nif)
{
    target.post(nif);
    data.post(nif);
}

void Nif::bhkConvexListShape::read(NIFStream *nif)
{
    numSubShapes = nif->getUInt();
    subShapes.resize(numSubShapes);
    for (unsigned int i = 0; i < numSubShapes; ++i)
        subShapes[i].read(nif);

    material = nif->getUInt();

    unknown.resize(6);
    for (int i = 0; i < 6; ++i)
        unknown[i] = nif->getFloat();
    nif->getChar();
    nif->getFloat();
}

void Nif::bhkConvexListShape::post(NIFFile *nif)
{
    for (unsigned int i = 0; i < subShapes.size(); ++i)
        subShapes[i].post(nif);
}

void Nif::bhkConstraint::read(NIFStream *nif)
{
    unsigned int numEntities = nif->getUInt();
    entities.resize(numEntities);
    for (unsigned int i = 0; i < numEntities; ++i)
    {
        entities[i].read(nif);
    }
    priority = nif->getUInt();
}

void Nif::bhkConstraint::post(NIFFile *nif)
{
    for (unsigned int i = 0; i < entities.size(); ++i)
        entities[i].post(nif);
}

void Nif::RagdollDescriptor::read(NIFStream *nif, unsigned int nifVer)
{
    if (nifVer <= 0x14000005)
    {
        pivotA = nif->getVector4();
        planeA = nif->getVector4();
        twistA = nif->getVector4();
        pivotB = nif->getVector4();
        planeB = nif->getVector4();
        twistB = nif->getVector4();
    }

    if (nifVer >= 0x14020007)
    {
        twistA = nif->getVector4();
        planeA = nif->getVector4();
        motorA = nif->getVector4();
        pivotA = nif->getVector4();
        twistB = nif->getVector4();
        planeB = nif->getVector4();
        motorB = nif->getVector4();
        pivotB = nif->getVector4();
    }

    coneMaxAngle = nif->getFloat();
    planeMinAngle = nif->getFloat();
    planeMaxAngle = nif->getFloat();
    twistMinAngle = nif->getFloat();
    twistMaxAngle = nif->getFloat();
    maxFriction = nif->getFloat();

    if (nifVer >= 0x14020007)
    {
        bool enableMotor = nif->getBool(nifVer);
        if (enableMotor)
        {
            nif->getFloat(); // unknown float 1
            nif->getFloat(); // unknown float 2
            nif->getFloat(); // unknown float 3
            nif->getFloat(); // unknown float 4
            nif->getFloat(); // unknown float 5
            nif->getFloat(); // unknown float 6
            nif->getChar();  // unknown byte 1
        }
    }
}

void Nif::bhkRagdollConstraint::read(NIFStream *nif)
{
    bhkConstraint::read(nif);
    ragdoll.read(nif, nifVer);
}

void Nif::LimitedHingeDescriptor::read(NIFStream *nif, unsigned int nifVer)
{
    if (nifVer <= 0x14000005)
    {
        pivotA = nif->getVector4();
        axleA = nif->getVector4();
        perp2AxleA1 = nif->getVector4();
        perp2AxleA2 = nif->getVector4();
        pivotB = nif->getVector4();
        axleB = nif->getVector4();
        perp2AxleB2 = nif->getVector4();
    }

    if (nifVer >= 0x14020007)
    {
        axleA = nif->getVector4();
        perp2AxleA1 = nif->getVector4();
        perp2AxleA2 = nif->getVector4();
        pivotA = nif->getVector4();
        axleB = nif->getVector4();
        perp2AxleB1 = nif->getVector4();
        perp2AxleB2 = nif->getVector4();
        pivotB = nif->getVector4();
    }

    minAngle = nif->getFloat();
    maxAngle = nif->getFloat();
    maxFriction = nif->getFloat();

    if (nifVer >= 0x14020007)
    {
        enableMotor = nif->getBool(nifVer);
        if (enableMotor)
        {
            nif->getFloat(); // unknown float 1
            nif->getFloat(); // unknown float 2
            nif->getFloat(); // unknown float 3
            nif->getFloat(); // unknown float 4
            nif->getFloat(); // unknown float 5
            nif->getFloat(); // unknown float 6
            nif->getChar();  // unknown byte 1
        }
    }
}

void Nif::bhkLimitedHingeConstraint::read(NIFStream *nif)
{
    bhkConstraint::read(nif);
    limitedHinge.read(nif, nifVer);
}

void Nif::HingeDescriptor::read(NIFStream *nif, unsigned int nifVer)
{
    if (nifVer <= 0x14000005)
    {
        pivotA = nif->getVector4();
        perp2AxleA1 = nif->getVector4();
        perp2AxleA2 = nif->getVector4();
        pivotB = nif->getVector4();
        axleB = nif->getVector4();
    }

    if (nifVer >= 0x14020007)
    {
        axleA = nif->getVector4();
        perp2AxleA1 = nif->getVector4();
        perp2AxleA2 = nif->getVector4();
        pivotA = nif->getVector4();
        axleB = nif->getVector4();
        perp2AxleB1 = nif->getVector4();
        perp2AxleB2 = nif->getVector4();
        pivotB = nif->getVector4();
    }
}

void Nif::bhkHingeConstraint::read(NIFStream *nif)
{
    bhkConstraint::read(nif);
    hinge.read(nif, nifVer);
}

void Nif::bhkPrismaticConstraint::read(NIFStream *nif)
{
    bhkConstraint::read(nif);

    if (nifVer <= 0x14000005)
    {
        pivotA = nif->getVector4();
        rotationMatrixA.resize(4);
        for (int i = 0; i < 4; ++i)
            rotationMatrixA[i] = nif->getVector4();
        pivotB = nif->getVector4();
        slidingB = nif->getVector4();
        planeB = nif->getVector4();
    }

    if (nifVer >= 0x14020007)
    {
        slidingA = nif->getVector4();
        rotationA = nif->getVector4();
        planeA = nif->getVector4();
        pivotA = nif->getVector4();
        slidingB = nif->getVector4();
        rotationB = nif->getVector4();
        planeB = nif->getVector4();
        pivotB = nif->getVector4();
    }

    minDistance = nif->getFloat();
    maxDistance = nif->getFloat();
    friction = nif->getFloat();

    if (nifVer >= 0x14020007)
        nif->getChar();
}

void Nif::bhkStiffSpringConstraint::read(NIFStream *nif)
{
    bhkConstraint::read(nif);

    pivotA = nif->getVector4();
    pivotB = nif->getVector4();
    length = nif->getFloat();
}

void Nif::bhkBreakableConstraint::read(NIFStream *nif)
{
    bhkConstraint::read(nif);

    if (userVer <= 11)
    {
        for (int i = 0; i < 41; ++i)
            nif->getInt();
        nif->getShort();
    }

    if (userVer == 12)
    {
        unknownI1 = nif->getUInt();

        unsigned int numEnt2 = nif->getUInt();
        entities2.resize(numEnt2);
        for (unsigned int i = 0; i < numEnt2; ++i)
            entities2[i].read(nif);

        priority2 = nif->getUInt();

        unknownI2 = nif->getUInt();
        position = nif->getVector3();
        rotation = nif->getVector3();
        unknownI3 = nif->getUInt();
        threshold = nif->getFloat();
        if (unknownI1 >= 1)
            unknownF1 = nif->getFloat();
        nif->getChar();
    }
}

void Nif::bhkBreakableConstraint::post(NIFFile *nif)
{
    for (unsigned int i = 0; i < entities2.size(); ++i)
        entities2[i].post(nif);
}

void Nif::bhkBallSocketConstraintChain::read(NIFStream *nif)
{
    unsigned int numFloats = nif->getUInt();
    nif->getVector4s(floats1, numFloats);
    unknownF1 = nif->getFloat();
    unknownF2 = nif->getFloat();
    unknownI1 = nif->getUInt();
    unknownI2 = nif->getUInt();

    unsigned int numLinks = nif->getUInt();
    links.resize(numLinks);
    for (unsigned int i = 0; i < numLinks; ++i)
        links[i].read(nif);

    numLinks = nif->getUInt(); // reused
    links2.resize(numLinks);
    for (unsigned int i = 0; i < numLinks; ++i)
        links2[i].read(nif);

    unknownI3 = nif->getUInt();
}

void Nif::bhkBallSocketConstraintChain::post(NIFFile *nif)
{
    for (unsigned int i = 0; i < links.size(); ++i)
        links[i].post(nif);

    for (unsigned int i = 0; i < links2.size(); ++i)
        links2[i].post(nif);
}

void Nif::bhkMalleableConstraint::read(NIFStream *nif)
{
    bhkConstraint::read(nif);

    type = nif->getUInt();
    unknown2 = nif->getUInt();
    link1.read(nif);
    link2.read(nif);
    unknown3 = nif->getUInt();

    if (type == 1)
        hinge.read(nif, nifVer);
    else if (type == 2)
        limitedHinge.read(nif, nifVer);
    else if (type == 7)
        ragdoll.read(nif, nifVer);

    if (nifVer <= 0x14000005) // up to 20.0.0.5
        tau = nif->getFloat();
    damping = nif->getFloat();
}

void Nif::bhkMalleableConstraint::post(NIFFile *nif)
{
    link1.post(nif);
    link2.post(nif);
}

void Nif::bhkEntity::read(NIFStream *nif)
{
    shape.read(nif);
    layer = nif->getChar();
    collisionFilter = nif->getChar();
    unknownShort = nif->getUShort();
}

void Nif::bhkEntity::post(NIFFile *nif)
{
    shape.post(nif);
}

void Nif::bhkRigidBody::read(NIFStream *nif)
{
    bhkEntity::read(nif);

    unknownInt1 = nif->getInt();
    unknownInt2 = nif->getInt();
    unknown3Ints.resize(3);
    for(size_t i = 0; i < 3; i++)
        unknown3Ints[i] = nif->getInt();
    collisionResponse = nif->getChar();
    unknownByte = nif->getChar();
    callbackDelay = nif->getUShort();
    unknown2 = nif->getUShort();
    unknown3 = nif->getUShort();
    layerCopy = nif->getChar();
    collisionFilterCopy = nif->getChar();
    unknown7Shorts.resize(7);
    for(size_t i = 0; i < 7; i++)
        unknown7Shorts[i] = nif->getUShort();

    translation = nif->getVector4();
    rotation = nif->getVector4();
    velocityLinear = nif->getVector4();
    velocityAngular = nif->getVector4();
    for(size_t i = 0; i < 3; i++)
    {
        for(size_t j = 0; j < 4; j++)
            inertia[i][j] = Ogre::Real(nif->getFloat());
    }
    center = nif->getVector4();
    mass = nif->getFloat();
    dampingLinear = nif->getFloat();
    dampingAngular = nif->getFloat();
    if (userVer >= 12)
    {
        gravityFactor1 = nif->getFloat();
        gravityFactor2 = nif->getFloat();
    }
    friction = nif->getFloat();
    if (userVer >= 12)
        rollingFrictionMultiplier = nif->getFloat();
    restitution = nif->getFloat();
    maxVelocityLinear = nif->getFloat();
    maxVelocityAngular = nif->getFloat();
    penetrationDepth = nif->getFloat();

    motionSystem = nif->getChar();
    deactivatorType = nif->getChar();
    solverDeactivation = nif->getChar();
    motionQuality = nif->getChar();

    unknownInt6 = nif->getInt();
    unknownInt7 = nif->getInt();
    unknownInt8 = nif->getInt();
    if (userVer >= 12)
        nif->getInt();
    unsigned int numConst = nif->getUInt();
    constraints.resize(numConst);
    for(size_t i = 0; i < numConst; i++)
        constraints[i].read(nif);
    if (userVer <= 11)
        unknownInt9 = nif->getInt();
    if (userVer >= 12)
        unknownS9 = nif->getUShort();
}

void Nif::bhkRigidBody::post(NIFFile *nif)
{
    bhkEntity::post(nif);

    for (unsigned int i = 0; i < constraints.size(); ++i)
        constraints[i].post(nif);
}

void Nif::NiCollisionObject::read(NIFStream *nif)
{
    target.read(nif);
}

void Nif::NiCollisionObject::post(NIFFile *nif)
{
    target.post(nif);
}

void Nif::NiCollisionData::read(NIFStream *nif)
{
    NiCollisionObject::read(nif);
    propagationMode = nif->getUInt();
    collisionMode = nif->getUInt();
    bool useABV = !!nif->getChar();
    if (useABV)
    {
        bv.collisionType = nif->getUInt();
        switch (bv.collisionType)
        {
            case 0:
                bv.sphere.center = nif->getVector3();
                bv.sphere.radius = nif->getFloat();
                break;
            case 1:
                bv.box.center = nif->getVector3();
                bv.box.axis.resize(3);
                for (int i = 0; i < 3; ++i)
                    bv.box.axis[i] = nif->getVector3();
                bv.box.extent.resize(3);
                for (int i = 0; i < 3; ++i)
                    bv.box.extent[i] = nif->getFloat();
                break;
            case 2:
                bv.capsule.center = nif->getVector3();
                bv.capsule.origin = nif->getVector3();
                bv.capsule.unknown1 = nif->getFloat();
                bv.capsule.unknown2 = nif->getFloat();
                break;
            case 5:
                bv.halfspace.normal = nif->getVector3();
                bv.halfspace.center = nif->getVector3();
                break;
            default:
                break;
        }
    }
}

void Nif::bhkNiCollisionObject::read(NIFStream *nif)
{
    NiCollisionObject::read(nif);
    flags = nif->getUShort();
    body.read(nif);
}

void Nif::bhkNiCollisionObject::post(NIFFile *nif)
{
    NiCollisionObject::post(nif);
    body.post(nif);
}

void Nif::bhkBlendCollisionObject::read(NIFStream *nif)
{
    bhkNiCollisionObject::read(nif);
    unknown1 = nif->getFloat();
    unknown2 = nif->getFloat();
}

void Nif::bhkSimpleShapePhantom::read(NIFStream *nif)
{
    shape.read(nif);
    oblivionLayer = nif->getChar();
    colFilter = nif->getChar();

    nif->getUShort();
    for (unsigned int i = 0; i < 23; ++i) // 7 + 3*5 +1
        nif->getFloat();
}

void Nif::bhkSimpleShapePhantom::post(NIFFile *nif)
{
    shape.post(nif);
}

void Nif::bhkCMSDChunk::read(NIFStream *nif, unsigned int nifVer)
{
    translation = nif->getVector4();
    materialIndex = nif->getUInt();
    unknown1 = nif->getUShort();
    transformIndex = nif->getUShort();
    unsigned int numVert = nif->getUInt();
    vertices.resize(numVert);
    for (unsigned int i = 0; i < numVert; ++i)
        vertices[i] = nif->getUShort();
    unsigned int numIndicies = nif->getUInt();
    indicies.resize(numIndicies);
    for (unsigned int i = 0; i < numIndicies; ++i)
        indicies[i] = nif->getUShort();
    unsigned int numStrips = nif->getUInt();
    strips.resize(numStrips);
    for (unsigned int i = 0; i < numStrips; ++i)
        strips[i] = nif->getUShort();
    unsigned int numIndicies2 = nif->getUInt();
    indicies2.resize(numIndicies2);
    for (unsigned int i = 0; i < numIndicies2; ++i)
        indicies2[i] = nif->getUShort();
}

void Nif::bhkCompressedMeshShapeData::read(NIFStream *nif)
{
    bitsPerIndex = nif->getUInt();
    bitsPerWIndex = nif->getUInt();
    maskWIndex = nif->getUInt();
    maskIndex = nif->getUInt();
    error = nif->getFloat();
    boundsMin = nif->getVector4();
    boundsMax = nif->getVector4();
    nif->getChar();
    nif->getInt();
    nif->getInt();
    nif->getInt();
    nif->getChar();
    unsigned int numMat = nif->getUInt();
    chunkMaterials.resize(numMat);
    for (unsigned int i = 0; i < numMat; ++i)
    {
        chunkMaterials[i].skyrimMaterial = nif->getUInt();
        chunkMaterials[i].unknown = nif->getUInt();
    }
    nif->getInt();
    unsigned int numTrans = nif->getUInt();
    chunkTransforms.resize(numTrans);
    for (unsigned int i = 0; i < numTrans; ++i)
    {
        chunkTransforms[i].translation = nif->getVector4();
        chunkTransforms[i].rotation = nif->getQuaternion();
    }
    unsigned int numBigVerts = nif->getUInt();
    bigVerts.resize(numBigVerts);
    for (unsigned int i = 0; i < numBigVerts; ++i)
        bigVerts[i] = nif->getVector4();
    unsigned int numBigTris = nif->getUInt();
    bigTris.resize(numBigTris);
    for (unsigned int i = 0; i < numBigTris; ++i)
    {
        bigTris[i].triangle1 = nif->getUShort();
        bigTris[i].triangle2 = nif->getUShort();
        bigTris[i].triangle3 = nif->getUShort();
        bigTris[i].unknown1 = nif->getUInt();
        bigTris[i].unknown2 = nif->getUShort();
    }
    unsigned int numChunks = nif->getUInt();
    chunks.resize(numChunks);
    for (unsigned int i = 0; i < numChunks; ++i)
        chunks[i].read(nif, nifVer);
    nif->getInt();
}
