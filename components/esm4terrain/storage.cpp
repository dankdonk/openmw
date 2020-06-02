#include "storage.hpp"

#include <set>
#include <iostream>
#include <cassert>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include <OgreVector2.h>
#include <OgreTextureManager.h>
#include <OgreStringConverter.h>
#include <OgreRenderSystem.h>
#include <OgreResourceGroupManager.h>
#include <OgreResourceBackgroundQueue.h>
#include <OgreRoot.h>
//
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreManualObject.h>
#include <OgreHardwarePixelBuffer.h>

#include <boost/algorithm/string.hpp>

#include <components/terrain/quadtreenode.hpp>
#include <components/misc/resourcehelpers.hpp>

#include <extern/esm4/land.hpp>
#include <extern/esm4/ltex.hpp>

// FIXME: this causes trouble with OpenCS
//#include "../../apps/openmw/mwbase/environment.hpp"
//#include "../../apps/openmw/mwbase/world.hpp"
//#include "../../apps/openmw/mwworld/esmstore.hpp"

#include "../terrain/quadtreenode.hpp"

//#include "../../apps/opencs/model/foreign/land.hpp" // FIXME: a bit ugly including it here...

// FIXME: copied the whole thing from quadtreenode.cpp
namespace
{
    using namespace Terrain;

    int Log2( int n )
    {
        assert(n > 0);
        int targetlevel = 0;
        while (n >>= 1) ++targetlevel;
        return targetlevel;
    }

    // Utility functions for neighbour finding algorithm
    ChildDirection reflect(ChildDirection dir, Direction dir2)
    {
        assert(dir != Root);

        const int lookupTable[4][4] =
        {
            // NW  NE  SW  SE
            {  SW, SE, NW, NE }, // N
            {  NE, NW, SE, SW }, // E
            {  SW, SE, NW, NE }, // S
            {  NE, NW, SE, SW }  // W
        };
        return (ChildDirection)lookupTable[dir2][dir];
    }

    bool adjacent(ChildDirection dir, Direction dir2)
    {
        assert(dir != Root);
        const bool lookupTable[4][4] =
        {
            // NW    NE    SW     SE
            {  true, true, false, false }, // N
            {  false, true, false, true }, // E
            {  false, false, true, true }, // S
            {  true, false, true, false }  // W
        };
        return lookupTable[dir2][dir];
    }

    // Algorithm described by Hanan Samet - 'Neighbour Finding in Quadtrees'
    // http://www.cs.umd.edu/~hjs/pubs/SametPRIP81.pdf
    QuadTreeNode* searchNeighbourRecursive (QuadTreeNode* currentNode, Direction dir)
    {
        if (!currentNode->getParent())
            return NULL; // Arrived at root node, the root node does not have neighbours

        QuadTreeNode* nextNode;
        if (adjacent(currentNode->getDirection(), dir))
            nextNode = searchNeighbourRecursive(currentNode->getParent(), dir);
        else
            nextNode = currentNode->getParent();

        if (nextNode && nextNode->hasChildren())
            return nextNode->getChild(reflect(currentNode->getDirection(), dir));
        else
            return NULL;
    }

    // Create a 2D quad
    void makeQuad(Ogre::SceneManager* sceneMgr, float left, float top, float right, float bottom, Ogre::MaterialPtr material)
    {
        Ogre::ManualObject* manual = sceneMgr->createManualObject();

        // Use identity view/projection matrices to get a 2d quad
        manual->setUseIdentityProjection(true);
        manual->setUseIdentityView(true);

        manual->begin(material->getName());

        float normLeft = left*2-1;
        float normTop = top*2-1;
        float normRight = right*2-1;
        float normBottom = bottom*2-1;

        manual->position(normLeft, normTop, 0.0);
        manual->textureCoord(0, 1);
        manual->position(normRight, normTop, 0.0);
        manual->textureCoord(1, 1);
        manual->position(normRight, normBottom, 0.0);
        manual->textureCoord(1, 0);
        manual->position(normLeft, normBottom, 0.0);
        manual->textureCoord(0, 0);

        manual->quad(0,1,2,3);

        manual->end();

        Ogre::AxisAlignedBox aabInf;
        aabInf.setInfinite();
        manual->setBoundingBox(aabInf);

        sceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(manual);
    }
}

namespace ESM4Terrain
{

    // land is not necessarily identified by x/y, also need world formid i.e. CSVForeign::TerrainStorage::mWorld
    const LandData *Storage::getLandData (int cellX, int cellY, int flags)
    {
        if (const Land *land = getLand (cellX, cellY))
            return land->getLandData (flags);

        return nullptr;
    }

    // ESM4Terrain::TerrainGrid::loadCell() calls this method
    bool Storage::getMinMaxQuadHeights(const Ogre::Vector2 &center, float &min, float &max, int quad)
    {
        Ogre::Vector2 origin = center - Ogre::Vector2(1/2.f, 1/2.f);

        int cellX = static_cast<int>(std::floor(origin.x));
        int cellY = static_cast<int>(std::floor(origin.y));

        int rowStart, colStart, rowEnd, colEnd;
        switch (quad)
        {
            case 0: // bottom left
            {
                rowStart = 0;
                colStart = 0;

                rowEnd = int(ESM4::Land::VERTS_PER_SIDE / 2) + 1; // int(33 / 2) + 1 = 17
                colEnd = int(ESM4::Land::VERTS_PER_SIDE / 2) + 1;

                break;
            }
            case 1: // bottom right
            {
                rowStart = 0;
                colStart = int(ESM4::Land::VERTS_PER_SIDE / 2); // 16, repeat the last of the left quad

                rowEnd = int(ESM4::Land::VERTS_PER_SIDE / 2) + 1; // 17
                colEnd = ESM4::Land::VERTS_PER_SIDE;

                break;
            }
            case 2: // top left
            {
                rowStart = int(ESM4::Land::VERTS_PER_SIDE / 2); // 16, repeat the last of bottom quad
                colStart = 0;

                rowEnd = ESM4::Land::VERTS_PER_SIDE;
                colEnd = int(ESM4::Land::VERTS_PER_SIDE / 2) + 1; // 17

                break;
            }
            case 3: // top right
            {
                rowStart = int(ESM4::Land::VERTS_PER_SIDE / 2); // 16
                colStart = int(ESM4::Land::VERTS_PER_SIDE / 2); // 16

                rowEnd = ESM4::Land::VERTS_PER_SIDE; // 33
                colEnd = ESM4::Land::VERTS_PER_SIDE; // 33

                break;
            }
            default:
                return false;
        }

        // FIXME: pass this in as a parameter rather than calling it 4 times?
        if (const LandData *data = getLandData (cellX, cellY, ESM4::Land::LAND_VHGT))
        {
            min = std::numeric_limits<float>::max();
            max = -std::numeric_limits<float>::max();
            for (int row = rowStart; row < rowEnd; ++row)
            {
                for (int col = colStart; col < colEnd; ++col)
                {
                    float h = data->mHeights[col*ESM4::Land::VERTS_PER_SIDE+row];
                    if (h > max)
                        max = h;

                    if (h < min)
                        min = h;
                }
            }
            return true;
        }

        return false;
    }

    // DefaultWorld::buildQuadTree() and TerrainGrid::loadCell(1, ...) calls this method
    bool Storage::getMinMaxHeights(float size, const Ogre::Vector2 &center, float &min, float &max)
    {
        assert (size <= 1 && "Storage::getMinMaxHeights, chunk size should be <= 1 cell");

        /// \todo investigate if min/max heights should be stored at load time in ESM4::Land instead

        Ogre::Vector2 origin = center - Ogre::Vector2(size/2.f, size/2.f);

        int cellX = static_cast<int>(std::floor(origin.x));
        int cellY = static_cast<int>(std::floor(origin.y));

        int rowStart = ((int)origin.x - cellX) * ESM4::Land::VERTS_PER_SIDE;
        int colStart = ((int)origin.y - cellY) * ESM4::Land::VERTS_PER_SIDE;

        int rowEnd = rowStart + (int)size * (ESM4::Land::VERTS_PER_SIDE-1) + 1;
        int colEnd = colStart + (int)size * (ESM4::Land::VERTS_PER_SIDE-1) + 1;

        if (const LandData *data = getLandData (cellX, cellY, ESM4::Land::LAND_VHGT))
        {
            min = std::numeric_limits<float>::max();
            max = -std::numeric_limits<float>::max();
            for (int row=rowStart; row<rowEnd; ++row)
            {
                for (int col=colStart; col<colEnd; ++col)
                {
                    float h = data->mHeights[col*ESM4::Land::VERTS_PER_SIDE+row];
                    if (h > max)
                        max = h;
                    if (h < min)
                        min = h;
                }
            }
            return true;
        }

        return false;
    }

    void Storage::fixNormal (Ogre::Vector3& normal, int cellX, int cellY, int col, int row)
    {
        while (col >= ESM4::Land::VERTS_PER_SIDE-1)
        {
            ++cellY;
            col -= ESM4::Land::VERTS_PER_SIDE-1;
        }
        while (row >= ESM4::Land::VERTS_PER_SIDE-1)
        {
            ++cellX;
            row -= ESM4::Land::VERTS_PER_SIDE-1;
        }
        while (col < 0)
        {
            --cellY;
            col += ESM4::Land::VERTS_PER_SIDE-1;
        }
        while (row < 0)
        {
            --cellX;
            row += ESM4::Land::VERTS_PER_SIDE-1;
        }

        if (const Land *data = getLand/*Data*/ (cellX, cellY/*, ESM4::Land::LAND_VNML*/))
        {
            normal.x = data->mVertNorm[col*ESM4::Land::VERTS_PER_SIDE*3+row*3];
            normal.y = data->mVertNorm[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+1];
            normal.z = data->mVertNorm[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+2];
            normal.normalise();
        }
        else
            normal = Ogre::Vector3(0,0,1);
    }

    void Storage::averageNormal(Ogre::Vector3 &normal, int cellX, int cellY, int col, int row)
    {
        Ogre::Vector3 n1,n2,n3,n4;
        fixNormal(n1, cellX, cellY, col+1, row);
        fixNormal(n2, cellX, cellY, col-1, row);
        fixNormal(n3, cellX, cellY, col, row+1);
        fixNormal(n4, cellX, cellY, col, row-1);
        normal = (n1+n2+n3+n4);
        normal.normalise();
    }

    void Storage::fixColour (Ogre::ColourValue& color, int cellX, int cellY, int col, int row)
    {
        if (col == ESM4::Land::VERTS_PER_SIDE-1)
        {
            ++cellY;
            col = 0;
        }
        if (row == ESM4::Land::VERTS_PER_SIDE-1)
        {
            ++cellX;
            row = 0;
        }

        if (const Land *data = getLand/*Data*/ (cellX, cellY/*, ESM4::Land::LAND_VCLR*/))
        {
            color.r = data->mVertColr[col*ESM4::Land::VERTS_PER_SIDE*3+row*3] / 255.f;
            color.g = data->mVertColr[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+1] / 255.f;
            color.b = data->mVertColr[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+2] / 255.f;
        }
        else
        {
            color.r = 1;
            color.g = 1;
            color.b = 1;
        }

    }

    void Storage::fillQuadVertexBuffers (const Ogre::Vector2& center, Terrain::Alignment align,
                                            std::vector<float>& positions,
                                            std::vector<float>& normals,
                                            std::vector<Ogre::uint8>& colours,
                                            int quad)
    {
        Ogre::Vector2 origin = center - Ogre::Vector2(1/2.f, 1/2.f); // cell grid

        // FIXME: for quads within a cell the concept of "startCell" doesn't really make sense
        int startCellX = static_cast<int>(std::floor(origin.x));
        int startCellY = static_cast<int>(std::floor(origin.y));

        size_t numVerts = static_cast<size_t>(ESM4::Land::VERTS_PER_SIDE / 2) + 1; // 17

        positions.resize(numVerts*numVerts*3);
        normals.resize(numVerts*numVerts*3);
        colours.resize(numVerts*numVerts*4);

        Ogre::Vector3 normal;
        Ogre::ColourValue color;

        float vertY = 0;
        float vertX = 0;

        // calls Foreign::TerrainStorage::getLand() or CSVForeign::TerrainStorage::getLand()
        const Land *land = getLand(startCellX, startCellY);

        int rowStart = 0;
        int colStart = 0;
        int rowEnd, colEnd;

        // Only relevant for chunks smaller than (contained in) one cell
        // FIXME: repeated code, make it a function
        // FIXME: how to ignore the repeat of left/bottom quad?
        switch (quad)
        {
            case 0: // bottom left
            {
                rowStart = 0;
                colStart = 0;

                rowEnd = int(ESM4::Land::VERTS_PER_SIDE / 2) + 1; // int(33 / 2) + 1 = 17
                colEnd = int(ESM4::Land::VERTS_PER_SIDE / 2) + 1;

                break;
            }
            case 1: // bottom right
            {
                rowStart = 0;
                colStart = int(ESM4::Land::VERTS_PER_SIDE / 2); // 16, repeat the last of the left quad

                rowEnd = int(ESM4::Land::VERTS_PER_SIDE / 2) + 1; // 17
                colEnd = ESM4::Land::VERTS_PER_SIDE;

                break;
            }
            case 2: // top left
            {
                rowStart = int(ESM4::Land::VERTS_PER_SIDE / 2); // 16, repeat the last of the bottom quad
                colStart = 0;

                rowEnd = ESM4::Land::VERTS_PER_SIDE;
                colEnd = int(ESM4::Land::VERTS_PER_SIDE / 2) + 1; // 17

                break;
            }
            case 3: // top right
            {
                rowStart = int(ESM4::Land::VERTS_PER_SIDE / 2); // 16
                colStart = int(ESM4::Land::VERTS_PER_SIDE / 2); // 16

                rowEnd = ESM4::Land::VERTS_PER_SIDE; // 33
                colEnd = ESM4::Land::VERTS_PER_SIDE; // 33

                break;
            }
            default:
                return; // FIXME: throw?
        }
        //rowStart += ((int)origin.x - startCellX) * ESM4::Land::VERTS_PER_SIDE;
        //colStart += ((int)origin.y - startCellY) * ESM4::Land::VERTS_PER_SIDE;
        //int rowEnd = rowStart + ESM4::Land::VERTS_PER_SIDE;
        //int colEnd = colStart + ESM4::Land::VERTS_PER_SIDE;

        vertY = 0;
        for (int col = colStart; col < colEnd; col += 1)
        {
            vertX = 0;
            for (int row = rowStart; row < rowEnd; row += 1)
            {
                // x
                //std::cout << "index " << static_cast<unsigned int>(vertX * numVerts * 3 + vertY * 3 + 0)
                    //<< "," << ((vertX / float(numVerts - 1) - 0.5f) * 2048) << std::endl;
                positions[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3 + 0)]
                    = ((vertX / float(numVerts - 1) - 0.5f) * 2048);
                // y
                positions[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3 + 1)]
                    = ((vertY / float(numVerts - 1) - 0.5f) * 2048);

                assert(row >= 0 && row < ESM4::Land::VERTS_PER_SIDE);
                assert(col >= 0 && col < ESM4::Land::VERTS_PER_SIDE);

                assert (vertX < numVerts);
                assert (vertY < numVerts);

                float height = -1024; // FIXME: where did this number come from?
                if (land && (land->mDataTypes & ESM4::Land::LAND_VHGT) != 0)
                    height = land->mLandData.mHeights[col*ESM4::Land::VERTS_PER_SIDE + row];
                // z
                //std::cout << "index " << col*33+row << "," << height << std::endl;
                positions[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3 + 2)] = height;

                if (land && (land->mDataTypes & ESM4::Land::LAND_VNML) != 0)
                {
                    normal.x = land->mVertNorm[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+0];
                    normal.y = land->mVertNorm[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+1];
                    normal.z = land->mVertNorm[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+2];
                    normal.normalise();
                }
                else
                    normal = Ogre::Vector3(0,0,1);

                // Normals apparently don't connect seamlessly between cells
                if (col == ESM4::Land::VERTS_PER_SIDE-1 || row == ESM4::Land::VERTS_PER_SIDE-1)
                    fixNormal(normal, startCellX, startCellY, col, row);

                // some corner normals appear to be complete garbage (z < 0)
                if ((row == 0 || row == ESM4::Land::VERTS_PER_SIDE-1)
                        && (col == 0 || col == ESM4::Land::VERTS_PER_SIDE-1))
                    averageNormal(normal, startCellX, startCellY, col, row);

                //assert(normal.z > 0); // ToddLand triggers this
                if (normal.z < 0) normal.z = 0;

                normals[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3 + 0)] = normal.x;
                normals[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3 + 1)] = normal.y;
                normals[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3 + 2)] = normal.z;

                if (land && (land->mDataTypes & ESM4::Land::LAND_VCLR) != 0)
                {
                    color.r = land->mVertColr[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+0] / 255.f;
                    color.g = land->mVertColr[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+1] / 255.f;
                    color.b = land->mVertColr[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+2] / 255.f;
                }
                else
                {
                    color.r = 1;
                    color.g = 1;
                    color.b = 1;
                }

                // Unlike normals, colors mostly connect seamlessly between cells, but not always...
                if (col == ESM4::Land::VERTS_PER_SIDE-1 || row == ESM4::Land::VERTS_PER_SIDE-1)
                    fixColour(color, startCellX, startCellY, col, row);

                color.a = 1;
                Ogre::uint32 rsColor;
                Ogre::Root::getSingleton().getRenderSystem()->convertColourValue(color, &rsColor);
                memcpy(&colours[static_cast<unsigned int>(vertX*numVerts * 4 + vertY * 4)],
                        &rsColor, sizeof(Ogre::uint32));

                ++vertX;
            }
            ++vertY;
        }
    }

    // DefaultWorld::handleRequest() and TerrainGrid::loadCell(0, 1, ...) calls this method
    void Storage::fillVertexBuffers (int lodLevel, float size, const Ogre::Vector2& center, Terrain::Alignment align,
                                            std::vector<float>& positions,
                                            std::vector<float>& normals,
                                            std::vector<Ogre::uint8>& colours)
    {
        // LOD level n means every 2^n-th vertex is kept
        size_t increment = 1 << lodLevel;

        Ogre::Vector2 origin = center - Ogre::Vector2(size/2.f, size/2.f);

        int startCellX = static_cast<int>(std::floor(origin.x));
        int startCellY = static_cast<int>(std::floor(origin.y));

        size_t numVerts = static_cast<size_t>(size*(ESM4::Land::VERTS_PER_SIDE - 1) / increment + 1);

        positions.resize(numVerts*numVerts*3);
        normals.resize(numVerts*numVerts*3);
        colours.resize(numVerts*numVerts*4);

        Ogre::Vector3 normal;
        Ogre::ColourValue color;

        float vertY = 0;
        float vertX = 0;

        float vertY_ = 0; // of current cell corner
        for (int cellY = startCellY; cellY < startCellY + std::ceil(size); ++cellY)
        {
            float vertX_ = 0; // of current cell corner
            for (int cellX = startCellX; cellX < startCellX + std::ceil(size); ++cellX)
            {
                const Land *heightData = getLand/*Data*/ (cellX, cellY/*, ESM4::Land::LAND_VHGT*/);
                const Land *normalData = getLand/*Data*/ (cellX, cellY/*, ESM4::Land::LAND_VNML*/);
                const Land *colourData = getLand/*Data*/ (cellX, cellY/*, ESM4::Land::LAND_VCLR*/);

                int rowStart = 0;
                int colStart = 0;
                // Skip the first row / column unless we're at a chunk edge,
                // since this row / column is already contained in a previous cell
                // This is only relevant if we're creating a chunk spanning multiple cells
                if (colStart == 0 && vertY_ != 0)
                    colStart += increment;
                if (rowStart == 0 && vertX_ != 0)
                    rowStart += increment;

                // Only relevant for chunks smaller than (contained in) one cell
                rowStart += ((int)origin.x - startCellX) * ESM4::Land::VERTS_PER_SIDE;
                colStart += ((int)origin.y - startCellY) * ESM4::Land::VERTS_PER_SIDE;
                int rowEnd = rowStart + (int)std::min(1.f, size) * (ESM4::Land::VERTS_PER_SIDE-1) + 1;
                int colEnd = colStart + (int)std::min(1.f, size) * (ESM4::Land::VERTS_PER_SIDE-1) + 1;

                vertY = vertY_;
                for (int col=colStart; col<colEnd; col += increment)
                {
                    vertX = vertX_;
                    for (int row=rowStart; row<rowEnd; row += increment)
                    {

                        //std::cout << "index " << static_cast<unsigned int>(vertX * numVerts * 3 + vertY * 3 + 0)
                            //<< "," << ((vertX / float(numVerts - 1) - 0.5f) * size * 4096) << std::endl;

                        positions[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3)]
                            = ((vertX / float(numVerts - 1) - 0.5f) * size * 4096);
                        positions[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3 + 1)]
                            = ((vertY / float(numVerts - 1) - 0.5f) * size * 4096);

                        assert(row >= 0 && row < ESM4::Land::VERTS_PER_SIDE);
                        assert(col >= 0 && col < ESM4::Land::VERTS_PER_SIDE);

                        assert (vertX < numVerts);
                        assert (vertY < numVerts);

                        float height = -1024;
                        if (heightData)
                            height = heightData->mLandData.mHeights[col*ESM4::Land::VERTS_PER_SIDE + row];
                        positions[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3 + 2)] = height;

                        if (normalData)
                        {
                            normal.x = normalData->mVertNorm[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+0];
                            normal.y = normalData->mVertNorm[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+1];
                            normal.z = normalData->mVertNorm[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+2];
                            normal.normalise();
                        }
                        else
                            normal = Ogre::Vector3(0,0,1);

                        // Normals apparently don't connect seamlessly between cells
                        if (col == ESM4::Land::VERTS_PER_SIDE-1 || row == ESM4::Land::VERTS_PER_SIDE-1)
                            fixNormal(normal, cellX, cellY, col, row);

                        // some corner normals appear to be complete garbage (z < 0)
                        if ((row == 0 || row == ESM4::Land::VERTS_PER_SIDE-1)
                                && (col == 0 || col == ESM4::Land::VERTS_PER_SIDE-1))
                            averageNormal(normal, cellX, cellY, col, row);

                        //assert(normal.z > 0); // ToddLand triggers this
                        if (normal.z < 0) normal.z = 0;

                        normals[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3 + 0)] = normal.x;
                        normals[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3 + 1)] = normal.y;
                        normals[static_cast<unsigned int>(vertX*numVerts * 3 + vertY * 3 + 2)] = normal.z;

                        if (colourData)
                        {
                            color.r = colourData->mVertColr[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+0] / 255.f;
                            color.g = colourData->mVertColr[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+1] / 255.f;
                            color.b = colourData->mVertColr[col*ESM4::Land::VERTS_PER_SIDE*3+row*3+2] / 255.f;
                        }
                        else
                        {
                            color.r = 1;
                            color.g = 1;
                            color.b = 1;
                        }

                        // Unlike normals, colors mostly connect seamlessly between cells, but not always...
                        if (col == ESM4::Land::VERTS_PER_SIDE-1 || row == ESM4::Land::VERTS_PER_SIDE-1)
                            fixColour(color, cellX, cellY, col, row);

                        color.a = 1;
                        Ogre::uint32 rsColor;
                        Ogre::Root::getSingleton().getRenderSystem()->convertColourValue(color, &rsColor);
                        memcpy(&colours[static_cast<unsigned int>(vertX*numVerts * 4 + vertY * 4)],
                                &rsColor, sizeof(Ogre::uint32));

                        ++vertX;
                    }
                    ++vertY;
                }
                vertX_ = vertX;
            }
            vertY_ = vertY;

            assert(vertX_ == numVerts); // Ensure we covered whole area
        }
        assert(vertY_ == numVerts);  // Ensure we covered whole area
    }

    // FIXME: seems to be retrieving which texture at a given point;
    //        not sure how that works for TES4 given that there can be up to 9 layers
    //
    // also, don't think we need to be fetching textures from neighbouring cells in TES4?
    //
    // param: cellX, cellY = cell grid
    //        x, y = texture pos in cell?
    Storage::UniqueTextureId Storage::getVtexIndexAt(int cellX, int cellY, int x, int y)
    {
#if 0 // TES3
        // For the first/last row/column, we need to get the texture from the neighbour cell
        // to get consistent blending at the borders
        --x;
        if (x < 0)
        {
            --cellX; // get neighbouring cell
            x += ESM::Land::LAND_TEXTURE_SIZE;
        }
        if (y >= ESM::Land::LAND_TEXTURE_SIZE) // Y appears to be wrapped from the other side because why the hell not?
        {
            ++cellY; // get neighbouring cell
            y -= ESM::Land::LAND_TEXTURE_SIZE;
        }

        assert(x < ESM::Land::LAND_TEXTURE_SIZE);
        assert(y < ESM::Land::LAND_TEXTURE_SIZE);

        if (const ESM::Land::LandData *data = getLandData (cellX, cellY, ESM::Land::DATA_VTEX))
        {
            int tex = data->mTextures[y * ESM::Land::LAND_TEXTURE_SIZE + x];
            if (tex == 0)
                return std::make_pair(0,0); // vtex 0 is always the base texture, regardless of plugin
            return std::make_pair(tex, getLand (cellX, cellY)->mPlugin);
        }
#else // TES4
        if (const Land *data = getLand(cellX, cellY))
        {
            int tex = 0;// data->mTextures[y * ESM4::Land::LAND_TEXTURE_SIZE + x]; // FIXME
            if (tex == 0)
                return std::make_pair(0,0); // no blending for base texture
            return std::make_pair(tex, 0);
        }
#endif
        else
            return std::make_pair(0,0);
    }

    std::string Storage::getTextureName(UniqueTextureId id) // pair<texture id, plugin id>
    {
        static const std::string defaultTexture = "textures\\_land_default.dds";
        if (id.first == 0)
            return defaultTexture; // Not sure if the default texture really is hardcoded?

        // NB: All vtex ids are +1 compared to the ltex ids
        const ESM4::LandTexture* ltex = getLandTexture(id.first/*, id.second*/);
        if (!ltex || ltex->mTextureFile.empty())
        {
            std::cerr << "Unable to find land texture index " << id.first-1
                << " in plugin " << id.second << ", using default texture instead" << std::endl;
            return defaultTexture;
        }

        // this is needed due to MWs messed up texture handling
        //std::string texture = Misc::ResourceHelpers::correctTexturePath(ltex->mTextureFile);

        //return texture;
        return ltex->mTextureFile;
    }

    void Storage::getBlendmaps (const std::vector<Terrain::QuadTreeNode*>& nodes,
            std::vector<Terrain::LayerCollection>& out, bool pack)
    {
        for (std::vector<Terrain::QuadTreeNode*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
        {
            out.push_back(Terrain::LayerCollection());
            out.back().mTarget = *it;
            getBlendmapsImpl(static_cast<float>((*it)->getSize()), (*it)->getCenter(), pack, out.back().mBlendmaps, out.back().mLayers);
        }
    }

    void Storage::getBlendmaps(float chunkSize, const Ogre::Vector2 &chunkCenter,
        bool pack, std::vector<Ogre::PixelBox> &blendmaps, std::vector<Terrain::LayerInfo> &layerList)
    {
        getBlendmapsImpl(chunkSize, chunkCenter, pack, blendmaps, layerList);
    }

    void Storage::getQuadBlendmaps(const Ogre::Vector2 &chunkCenter,
        bool pack, std::vector<Ogre::PixelBox> &blendmaps, std::vector<Terrain::LayerInfo> &layerList, int quad)
    {
        // VTXT info indicates texture size is 17x17 - but the cell grid is 33x33?
        // (cf. TES3 has 65x65 cell)
        // do we discard one row and column?

        //     ///////////////// ////////////////   <-- discard texture row?
        //    +-----------------+----------------+/
        // 32 |\                |                |/
        // 31 |\                |                |/
        //    |\     17x16      |      16x16     |/
        //  . |\                |                |/
        //  . |\       2        |        3       |/
        //  . |\                |                |/
        //  . |\                |                |/
        // 17 |\                |                |/
        //    +-----------------+----------------+
        // 16 |\               X|                |/ <-- X = chunkCenter (33/2 = 16.5)
        // 15 |\                |                |/
        //  . |\     17x17      |      16x17     |/
        //  . |\                |                |/
        //  . |\       0        |        1       |/
        //  . |\                |                |/
        //  2 |\                |                |/
        //  1 |\                |                |/
        //  0 |\\\\\\\\\\\\\\\\\|\\\\\\\\\\\\\\\\|/ <-- this row of vertices is a copy of cell below
        //    +-----------------+----------------+
        //                   111 1             33 ^
        //     0123  ......  456 7    .....    12 |
        //     ^                                 discard texture column?
        //     |
        //    this column of vertices is a copy of the cell to the left
        //

        // just for some helper functions
        //if (!mRootNode)
            //mRootNode = new Terrain::QuadTreeNode(nullptr, Terrain::Root, 1.f, Ogre::Vector2(0.5f, 0.5f), NULL);

        Ogre::Vector2 origin = chunkCenter - Ogre::Vector2(1/2.f, 1/2.f);
        int cellX = static_cast<int>(std::floor(origin.x));
        int cellY = static_cast<int>(std::floor(origin.y));

        // 16 textures per side of land for Morrowind and 2 textures per side for Oblivion?
        int realTextureSize = 16/*ESM4::Land::LAND_TEXTURE_SIZE*/+1; // add 1 to wrap around next cell

        // for TES3, chunkSize 1, chunkCenter (12.5, 1.5):
        // cellX = 12
        // cellY = 1
        int rowStart = ((int)origin.x - cellX) * realTextureSize; // should be 0 if chunkSize == 1
        int colStart = ((int)origin.y - cellY) * realTextureSize; // should be 0 if chunkSize == 1
        int rowEnd = rowStart + 1/*(int)chunkSize*/ * (realTextureSize-1) + 1; // realTextureSize if chunkSize == 1
        int colEnd = colStart + 1/*(int)chunkSize*/ * (realTextureSize-1) + 1; // realTextureSize if chunkSize == 1

        // Texture       mTextures[4]; // 0 = bottom left. 1 = bottom right. 2 = upper-left. 3 = upper-right
        //
        //    +-----------------+----------------+
        // 32 |\                |                |
        // 31 |\                |                |
        //    |\                |                |
        //  . |\                |                |
        //  . |\       2        |        3       |
        //  . |\                |                |
        //  . |\                |                |
        // 17 |\                |                |
        //    +-----------------+----------------+
        // 16 |\                |                |
        // 15 |\                |                |
        //  . |\                |                |
        //  . |\                |                |
        //  . |\       0        |        1       |
        //  . |\                |                |
        //  2 |\                |                |
        //  1 |\                |                |
        //  0 |\\\\\\\\\\\\\\\\\|\\\\\\\\\\\\\\\\| <-- this row is a copy of cell below
        //    +-----------------+----------------+
        //                   111 1             33
        //     0123  ......  456 7    .....    12
        //     ^
        //     |
        //    this column is a copy of the cell to the left
        //
        const ESM4::Land *land = getLand (cellX, cellY); // Foreign::TerrainStorage knows the world
        if (!land)
            return; // FIXME maybe return default textures?  throw an exception? assert?

        //std::cout << "land " << ESM4::formIdToString(land->mFormId) << std::endl; // FIXME

        // NOTE:  each base texture does not completely "fill" a quadrant.  Rather, the
        // textures repeat (or "wrap") 3 times each side based on observations with vanilla
        std::string bottomLeft = ESM4::formIdToString(land->mTextures[0].base.formId);
        std::string bottomRight = ESM4::formIdToString(land->mTextures[1].base.formId);
        std::string topLeft = ESM4::formIdToString(land->mTextures[2].base.formId);
        std::string topRight = ESM4::formIdToString(land->mTextures[3].base.formId);
        std::string uniqueName = "terrain/"+bottomLeft+"_"+bottomRight+"_"+topLeft+"_"+topRight;

        //std::string uniqueBase;
        //if (land->mTextures[quad].base.formId != 0)
        //    uniqueBase = "terrain/"+ESM4::formIdToString(land->mTextures[quad].base.formId);
        //else
        //    uniqueBase = "terrain/"+ESM4::formIdToString(0x000008C0); // TerrainHDDirt01.dds

        Ogre::TexturePtr baseTex = Ogre::TextureManager::getSingleton().getByName(
               uniqueName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        //const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        std::string blTextureFile;
        if (!baseTex)
        {
            const ESM4::LandTexture *blTex
                //= store.getForeign<ESM4::LandTexture>().search(land->mTextures[0].base.formId);
                = getLandTexture(land->mTextures[0].base.formId);
            const ESM4::LandTexture *brTex
                = getLandTexture(land->mTextures[1].base.formId);
            const ESM4::LandTexture *tlTex
                = getLandTexture(land->mTextures[2].base.formId);
            const ESM4::LandTexture *trTex
                = getLandTexture(land->mTextures[3].base.formId);
            if (!trTex)
                trTex = getLandTexture(0x000008C0);
            blTextureFile = "textures\\landscape\\"+blTex->mTextureFile;
            std::string brTextureFile = "textures\\landscape\\"+brTex->mTextureFile;
            std::string tlTextureFile = "textures\\landscape\\"+tlTex->mTextureFile;
            std::string trTextureFile = "textures\\landscape\\"+trTex->mTextureFile;

            // WARN: assume that all 4 base textures have the same type and dimensions
            Ogre::TexturePtr blTexture
                = Ogre::static_pointer_cast<Ogre::Texture>(Ogre::TextureManager::getSingleton().createOrRetrieve(
                   blTextureFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first);
            blTexture->load();
            Ogre::TexturePtr brTexture
                = Ogre::static_pointer_cast<Ogre::Texture>(Ogre::TextureManager::getSingleton().createOrRetrieve(
                   brTextureFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first);
            brTexture->load();
            Ogre::TexturePtr tlTexture
                = Ogre::static_pointer_cast<Ogre::Texture>(Ogre::TextureManager::getSingleton().createOrRetrieve(
                   tlTextureFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first);
            tlTexture->load();
            Ogre::TexturePtr trTexture
                = Ogre::static_pointer_cast<Ogre::Texture>(Ogre::TextureManager::getSingleton().createOrRetrieve(
                   trTextureFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME).first);
            trTexture->load();

            //if (!blTexture)
            //{
            //    std::cout << "couldn't create trial texture" << std::endl;
            //    return; // FIXME: throw?
            //}

            // create a blank one
            baseTex = Ogre::TextureManager::getSingleton().createManual(
                uniqueName, // name
                Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                blTexture->getTextureType(),  // type
                //blTexture->getWidth()*2, blTexture->getHeight()*2, // width & height
                blTexture->getWidth(), blTexture->getHeight(), // width & height
                1, // depth
                0,                  // number of mipmaps; FIXME: should be 2? or 1?
                //blTexture->getFromat(),
                Ogre::PF_A8R8G8B8,
                //Ogre::TU_DYNAMIC_WRITE_ONLY);
                //Ogre::TU_STATIC);
                Ogre::TU_DEFAULT);

            Ogre::TexturePtr srcTextureBL = Ogre::TextureManager::getSingleton().getByName(
                   "tempBufferBL",
                   Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (!srcTextureBL)
            {
                srcTextureBL = Ogre::TextureManager::getSingleton().createManual(
                    "tempBufferBL",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    blTexture->getTextureType(),  // type
                    blTexture->getWidth(), blTexture->getHeight(), // width & height
                    1, // depth
                    0,
                    //blTexture->getFormat(),
                    //Ogre::PF_BYTE_RGBA,
                    Ogre::PF_A8R8G8B8,
                    Ogre::TU_DEFAULT);
            }

            Ogre::TexturePtr srcTextureBR = Ogre::TextureManager::getSingleton().getByName(
                   "tempBufferBR",
                   Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (!srcTextureBR)
            {
                srcTextureBR = Ogre::TextureManager::getSingleton().createManual(
                    "tempBufferBR",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    brTexture->getTextureType(),  // type
                    brTexture->getWidth(), blTexture->getHeight(), // width & height
                    0,
                    Ogre::PF_BYTE_RGBA,
                    Ogre::TU_STATIC);
            }

            Ogre::TexturePtr srcTextureTL = Ogre::TextureManager::getSingleton().getByName(
                   "tempBufferTL",
                   Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (!srcTextureTL)
            {
                srcTextureTL = Ogre::TextureManager::getSingleton().createManual(
                    "tempBufferTL",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    tlTexture->getTextureType(),  // type
                    tlTexture->getWidth(), blTexture->getHeight(), // width & height
                    0,
                    Ogre::PF_BYTE_RGBA,
                    Ogre::TU_STATIC);
            }

            Ogre::TexturePtr srcTextureTR = Ogre::TextureManager::getSingleton().getByName(
                   "tempBufferTR",
                   Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (!srcTextureTR)
            {
                srcTextureTR = Ogre::TextureManager::getSingleton().createManual(
                    "tempBufferTR",
                    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                    trTexture->getTextureType(),  // type
                    trTexture->getWidth(), blTexture->getHeight(), // width & height
                    0,
                    Ogre::PF_BYTE_RGBA,
                    Ogre::TU_STATIC);
            }

            Ogre::HardwarePixelBufferSharedPtr pixelBufferDest = baseTex->getBuffer();
            //pixelBufferDest->unlock(); // prepare for blit()
            //pixelBufferDest->blit(blTexture->getBuffer()); // FIXME: temp testing

            Ogre::HardwarePixelBufferSharedPtr pixelBufferSrcBL = srcTextureBL->getBuffer();
            pixelBufferSrcBL->unlock(); // prepare for blit()
            pixelBufferSrcBL->blit(blTexture->getBuffer());

            Ogre::HardwarePixelBufferSharedPtr pixelBufferSrcBR = srcTextureBR->getBuffer();
            pixelBufferSrcBR->unlock(); // prepare for blit()
            pixelBufferSrcBR->blit(brTexture->getBuffer());

            Ogre::HardwarePixelBufferSharedPtr pixelBufferSrcTL = srcTextureTL->getBuffer();
            pixelBufferSrcTL->unlock(); // prepare for blit()
            pixelBufferSrcTL->blit(tlTexture->getBuffer());

            Ogre::HardwarePixelBufferSharedPtr pixelBufferSrcTR = srcTextureTR->getBuffer();
            pixelBufferSrcTR->unlock(); // prepare for blit()
            pixelBufferSrcTR->blit(trTexture->getBuffer());

            // Lock the pixel buffer and get a pixel box
            pixelBufferDest->lock(Ogre::HardwareBuffer::HBL_NORMAL);
            pixelBufferSrcBL->lock(Ogre::HardwareBuffer::HBL_NORMAL);
            pixelBufferSrcBR->lock(Ogre::HardwareBuffer::HBL_NORMAL);
            pixelBufferSrcTL->lock(Ogre::HardwareBuffer::HBL_NORMAL);
            pixelBufferSrcTR->lock(Ogre::HardwareBuffer::HBL_NORMAL);

            const Ogre::PixelBox& pixelBoxDest = pixelBufferDest->getCurrentLock();
            const Ogre::PixelBox& pixelBoxSrcBL = pixelBufferSrcBL->getCurrentLock();
            const Ogre::PixelBox& pixelBoxSrcBR = pixelBufferSrcBR->getCurrentLock();
            const Ogre::PixelBox& pixelBoxSrcTL = pixelBufferSrcTL->getCurrentLock();
            const Ogre::PixelBox& pixelBoxSrcTR = pixelBufferSrcTR->getCurrentLock();

            uint32_t *pDest = static_cast<uint32_t*>(pixelBoxDest.data);
            uint32_t *pSrcBL = static_cast<uint32_t*>(pixelBoxSrcBL.data);
            uint8_t *pSrcBR = static_cast<uint8_t*>(pixelBoxSrcBR.data);
            uint8_t *pSrcTL = static_cast<uint8_t*>(pixelBoxSrcTL.data);
            uint8_t *pSrcTR = static_cast<uint8_t*>(pixelBoxSrcTR.data);

            size_t quadHeight = blTexture->getHeight();
            size_t quadWidth = blTexture->getWidth();
            std::cout << blTextureFile << " height " << quadHeight << ", width " << quadWidth << std::endl;
            //size_t count = 0;

            //uint8_t *src;

            // pixel box starts at top left (do we need to transpose the textures?)
            //for (size_t y = 0; y < quadHeight*2; ++y)
            for (size_t y = 0; y < quadHeight; ++y)
            {
                //for (size_t x = 0; x < quadWidth*2; ++x)
                for (size_t x = 0; x < quadWidth; ++x)
                {
                    //if (y < quadHeight && x < quadWidth)
                    //    src = pSrcTL;
                    //else if (y < quadHeight)
                    //    src = pSrcTR;
                    //else if (x < quadWidth)
                    //    src = pSrcBL;
                    //else
                    //    src = pSrcBR;

                    *pDest++ = *pSrcBL++;
                    //*(pDest+0) = *(pSrcBL+0); // B
                    //*(pDest+1) = *(pSrcBL+1); // G
                    //*(pDest+2) = *(pSrcBL+2); // R
                    //*(pDest+3) = *(pSrcBL+3); // A
                    //pDest += 4;
                    //src += 4;
                    //count++;
                    //if (count >= ((0x200*0x200)-2))
                        //std::cout << "count " << count << " "<< x << "," << y << std::endl;
                }
            }

            pixelBufferDest->unlock();
            pixelBufferSrcBL->unlock();
            pixelBufferSrcBR->unlock();
            pixelBufferSrcTL->unlock();
            pixelBufferSrcTR->unlock();

            //baseTex->load();




#if 0
            pixelBufferDest->blit(pixelBufferSrc, Ogre::Box(0, blTexture->getHeight(), blTexture->getWidth(), 0),
                    Ogre::Box(0, blTexture->getHeight(), blTexture->getWidth(), 0));
            pixelBufferSrc->lock(Ogre::HardwareBuffer::HBL_DISCARD);

            pixelBufferSrc = brTexture->getBuffer();
            pixelBufferSrc->unlock(); // prepare for blit()

            pixelBufferDest->blit(pixelBufferSrc, Ogre::Box(0, brTexture->getHeight(), brTexture->getWidth(), 0),
                    Ogre::Box(blTexture->getWidth(), blTexture->getHeight(), blTexture->getWidth()*2, 0));

            pixelBufferSrc = tlTexture->getBuffer();
            pixelBufferSrc->unlock(); // prepare for blit()

            pixelBufferDest->blit(pixelBufferSrc, Ogre::Box(0, tlTexture->getHeight(), tlTexture->getWidth(), 0),
                    Ogre::Box(0, blTexture->getHeight()*2, blTexture->getWidth(), blTexture->getHeight()));

            pixelBufferSrc = trTexture->getBuffer();
            pixelBufferSrc->unlock(); // prepare for blit()

            pixelBufferDest->blit(pixelBufferSrc, Ogre::Box(0, trTexture->getHeight(), trTexture->getWidth(), 0),
                    Ogre::Box(blTexture->getWidth(), blTexture->getHeight()*2,
                              blTexture->getWidth()*2, blTexture->getHeight()));

            pixelBufferDest->lock(Ogre::HardwareBuffer::HBL_READ_ONLY);
#endif
        }

        // TES4: we're going to use BTXT instead of _land_default.dds
        std::set<UniqueTextureId> textureIndices;
        textureIndices.insert(std::make_pair(0,0));

        // 17x17 for chunkSize==1, see above notes
        //
        // 4*16+1=65 <- what is the relationship with the cell size?
        // maybe TES3 allows different texture for each 4x4 vertices?
        //
        // but if so we need to change to use different texture for each 16x16 vertices
        // (of up to 9 layers)
        // y 0..16 and x 0..16 for chunkSize == 1
        //
        // typedef std::pair<std::uint32_t, short> UniqueTextureId;
        //                       ^            ^
        //                       |            |
        //                 texture index  plugin index
#if 0
#endif
        //Ogre::MaterialPtr material = mMaterialGenerator->generateForCompositeMapRTT();
#if 0
        static int count = 0;
        std::stringstream name;
        name << "tes4terrain/mat" << count++;

        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create(name.str(),
                                                           Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        Ogre::Technique* technique = mat->getTechnique(0);
        technique->removeAllPasses();

        //assert(mLayerList.size() == mBlendmapList.size()+1);
        //std::vector<Ogre::TexturePtr>::iterator blend = mBlendmapList.begin();
        //for (std::vector<LayerInfo>::iterator layer = mLayerList.begin(); layer != mLayerList.end(); ++layer)
        {
            Ogre::Pass* pass = technique->createPass();
            pass->setLightingEnabled(false);
            pass->setVertexColourTracking(Ogre::TVC_NONE);
            // TODO: How to handle fog?
            pass->setFog(true, Ogre::FOG_NONE);

            //bool first = (layer == mLayerList.begin());

            Ogre::TextureUnitState* tus;

            // Add the actual layer texture on top of the alpha map.
            // TES4: diffuse tus created
            tus = pass->createTextureUnitState(layer->mDiffuseMap); // FIXME: get the texture from land

            // TES4: why scale at 1/16? is it because 16 textures per side are used?
            // if so won't the scaled down texture positions be incorrect? (maybe the
            // blend maps take care of that?)
            //tus->setTextureScale(1/16.f,1/16.f);

        }

        if (1)
        {
            Ogre::Pass* lightingPass = technique->createPass();
            lightingPass->setSceneBlending(Ogre::SBT_MODULATE);
            lightingPass->setVertexColourTracking(Ogre::TVC_AMBIENT|Ogre::TVC_DIFFUSE);
            lightingPass->setFog(true, Ogre::FOG_NONE);
        }
#endif
        //Ogre::TRect<float> area(0,0,1,1)
        //makeQuad(sceneMgr, area.left, area.top, area.right, area.bottom, mat);
        const ESM4::LandTexture *baseTexture;
        if (land->mTextures[quad].base.formId != 0)
            baseTexture = getLandTexture(land->mTextures[quad].base.formId);
        else
            baseTexture = getLandTexture(0x000008C0); // TerrainHDDirt01.dds
        std::string baseTextureFile = "textures\\landscape\\"+baseTexture->mTextureFile;

        std::map<UniqueTextureId, int> textureIndicesMap;
        for (std::set<UniqueTextureId>::iterator it = textureIndices.begin(); it != textureIndices.end(); ++it)
        {
            int size = (int)textureIndicesMap.size();
            textureIndicesMap[*it] = size;
            // NOTE: layerList is one of the parameters getBlendmapsImpl()
            //
            // getLayerInfo() returns Terrain::LayerInfo
            //
            // struct Terrain::LayerInfo
            // {
            //     std::string mDiffuseMap;
            //     std::string mNormalMap;
            //     bool mParallax; // Height info in normal map alpha channel?
            //     bool mSpecular; // Specular info in diffuse map alpha channel?
            // };
            //
            // but I think TES4 textures are organised differently?
            // where are the parallax and specula textures?  are they one of the 9 layers?
            // looking at the diffuse texture there isn't an alpha channel (which might have
            // been used for parallax), same with normal texture
            //
            if (it == textureIndices.begin())
            {
                Terrain::LayerInfo li;

                li.mDiffuseMap = baseTextureFile;
                //li.mDiffuseMap = uniqueName;
                //li.mDiffuseMap = blTextureFile;
                //li.mDiffuseMap = "tempBufferBL";
                li.mNormalMap = ""; // FIXME
                li.mParallax = false;
                li.mSpecular = false;
                li.mIsTes4 = true;

                layerList.push_back(li);
            }
            else
                layerList.push_back(getLayerInfo(getTextureName(*it)));
        }
        int numTextures = (int)textureIndices.size();
        int numBlendmaps = pack ? static_cast<int>(std::ceil((numTextures - 1) / 4.f)) : (numTextures - 1);

        int channels = pack ? 4 : 1;
        // FIXME: what is the logic behind blendmapSize? TES3: blendmapsize = (17-1)*1 + 1 = 17
        //
        const int blendmapSize = (realTextureSize-1) * 1/*(int)chunkSize*/ + 1;

        for (int i=0; i<numBlendmaps; ++i)
        {
            Ogre::PixelFormat format = pack ? Ogre::PF_A8B8G8R8 : Ogre::PF_A8;
            //                                         ^ ^ ^ ^             ^
            //                                         4 channels          1 channel

            Ogre::uchar* pData =
                            OGRE_ALLOC_T(Ogre::uchar, blendmapSize*blendmapSize*channels, Ogre::MEMCATEGORY_GENERAL);
            memset(pData, 0, blendmapSize*blendmapSize*channels);

            // y 0..16 and x 0..16 for chunkSize == 1 (i.e. 17x17)
            for (int y=0; y<blendmapSize; ++y)
            {
                for (int x=0; x<blendmapSize; ++x)
                {
                    // FIXME: why get this again?
                    UniqueTextureId id = getVtexIndexAt(cellX, cellY, x+rowStart, y+colStart);

                    assert(textureIndicesMap.find(id) != textureIndicesMap.end());
                    int layerIndex = textureIndicesMap.find(id)->second;
                    int blendIndex = (pack ? static_cast<int>(std::floor((layerIndex - 1) / 4.f)) : layerIndex - 1);
                    int channel = pack ? std::max(0, (layerIndex-1) % 4) : 0;

                    if (blendIndex == i)
                        pData[y*blendmapSize*channels + x*channels + channel] = 255;
                    else
                        pData[y*blendmapSize*channels + x*channels + channel] = 0;
                }
            }
            //                                 width         height        depth
            blendmaps.push_back(Ogre::PixelBox(blendmapSize, blendmapSize, 1, format, pData));
        }
    }

    // called by Terrain::TerrainGrid::loadCell()
    // for COW "Tamriel" 12 1,  chunkSize is 1.0, chunkCenter is (11.5, 0.5)
    void Storage::getBlendmapsImpl(float chunkSize, const Ogre::Vector2 &chunkCenter,
        bool pack, std::vector<Ogre::PixelBox> &blendmaps, std::vector<Terrain::LayerInfo> &layerList)
    {
        // NOTE: below is some comment from ESMTerrain::Storage
        /*
        // TODO - blending isn't completely right yet; the blending radius appears to be
        // different at a cell transition (2 vertices, not 4), so we may need to create a larger blendmap
        // and interpolate the rest of the cell by hand? :/
        */

        Ogre::Vector2 origin = chunkCenter - Ogre::Vector2(chunkSize/2.f, chunkSize/2.f);
        int cellX = static_cast<int>(std::floor(origin.x));
        int cellY = static_cast<int>(std::floor(origin.y));

        int realTextureSize = 16/*ESM4::Land::LAND_TEXTURE_SIZE*/+1; // add 1 to wrap around next cell

        int rowStart = ((int)origin.x - cellX) * realTextureSize;
        int colStart = ((int)origin.y - cellY) * realTextureSize;
        int rowEnd = rowStart + (int)chunkSize * (realTextureSize-1) + 1;
        int colEnd = colStart + (int)chunkSize * (realTextureSize-1) + 1;

        assert (rowStart >= 0 && colStart >= 0);
        assert (rowEnd <= realTextureSize);
        assert (colEnd <= realTextureSize);

        const ESM4::Land *land = getLand (cellX, cellY); // FIXME: support world
        if (!land)
            return; // FIXME maybe return default textures?  throw an exception? assert?

        // ESM4::LandTexture::mTextureFile can be obtained from Data::getForeignLandTextures()
        // using mTextures[quad].additional.formId
        //
        // CSVForeign::TerrainStorage::getLandTexture(ESM4::FormId formId, short plugin)


        // Q: how to use mTextures[quad].data[point].opacity
        // Q: each quad may have a different number of layers and in different order as well
        // Q: also each quad may have a differnt base texture
        // maybe TerrainGrid class itself may need to change?





        // Save the used texture indices so we know the total number of textures
        // and number of required blend maps
        std::set<UniqueTextureId> textureIndices;
        // Due to the way the blending works, the base layer will always shine through in
        // between blend transitions (eg halfway between two texels, both blend values will be
        // 0.5, so 25% of base layer visible).
        //
        // To get a consistent look, we need to make sure to use the same base layer in all
        // cells.  So we're always adding _land_default.dds as the base layer here, even if
        // it's not referenced in this cell.
        textureIndices.insert(std::make_pair(0,0));

        for (int y=colStart; y<colEnd; ++y)
            for (int x=rowStart; x<rowEnd; ++x)
            {
                UniqueTextureId id = getVtexIndexAt(cellX, cellY, x, y);
                textureIndices.insert(id);
            }

        // Makes sure the indices are sorted, or rather,
        // retrieved as sorted. This is important to keep the splatting order
        // consistent across cells.
        std::map<UniqueTextureId, int> textureIndicesMap;
        for (std::set<UniqueTextureId>::iterator it = textureIndices.begin(); it != textureIndices.end(); ++it)
        {
            int size = (int)textureIndicesMap.size();
            textureIndicesMap[*it] = size;
            layerList.push_back(getLayerInfo(getTextureName(*it)));
        }

        int numTextures = (int)textureIndices.size();
        // numTextures-1 since the base layer doesn't need blending
        int numBlendmaps = pack ? static_cast<int>(std::ceil((numTextures - 1) / 4.f)) : (numTextures - 1);

        int channels = pack ? 4 : 1;

        // Second iteration - create and fill in the blend maps
        const int blendmapSize = (realTextureSize-1) * (int)chunkSize + 1;

        for (int i=0; i<numBlendmaps; ++i)
        {
            Ogre::PixelFormat format = pack ? Ogre::PF_A8B8G8R8 : Ogre::PF_A8;

            Ogre::uchar* pData =
                            OGRE_ALLOC_T(Ogre::uchar, blendmapSize*blendmapSize*channels, Ogre::MEMCATEGORY_GENERAL);
            memset(pData, 0, blendmapSize*blendmapSize*channels);

            for (int y=0; y<blendmapSize; ++y)
            {
                for (int x=0; x<blendmapSize; ++x)
                {
                    UniqueTextureId id = getVtexIndexAt(cellX, cellY, x+rowStart, y+colStart);
                    assert(textureIndicesMap.find(id) != textureIndicesMap.end());
                    int layerIndex = textureIndicesMap.find(id)->second;
                    int blendIndex = (pack ? static_cast<int>(std::floor((layerIndex - 1) / 4.f)) : layerIndex - 1);
                    int channel = pack ? std::max(0, (layerIndex-1) % 4) : 0;

                    if (blendIndex == i)
                        pData[y*blendmapSize*channels + x*channels + channel] = 255;
                    else
                        pData[y*blendmapSize*channels + x*channels + channel] = 0;
                }
            }
            blendmaps.push_back(Ogre::PixelBox(blendmapSize, blendmapSize, 1, format, pData));
        }
    }

    float Storage::getHeightAt(const Ogre::Vector3 &worldPos)
    {
        int cellX = static_cast<int>(std::floor(worldPos.x / 4096.f));
        int cellY = static_cast<int>(std::floor(worldPos.y / 4096.f));

        const Land* land = getLand(cellX, cellY);
        if (!land || !(land->mDataTypes & ESM4::Land::LAND_VHGT))
            return -1024;

        // Mostly lifted from Ogre::Terrain::getHeightAtTerrainPosition

        // Normalized position in the cell
        float nX = (worldPos.x - (cellX * 4096))/4096.f;
        float nY = (worldPos.y - (cellY * 4096))/4096.f;

        // get left / bottom points (rounded down)
        float factor = ESM4::Land::VERTS_PER_SIDE - 1.0f;
        float invFactor = 1.0f / factor;

        int startX = static_cast<int>(nX * factor);
        int startY = static_cast<int>(nY * factor);
        int endX = startX + 1;
        int endY = startY + 1;

        endX = std::min(endX, ESM4::Land::VERTS_PER_SIDE-1);
        endY = std::min(endY, ESM4::Land::VERTS_PER_SIDE-1);

        // now get points in terrain space (effectively rounding them to boundaries)
        float startXTS = startX * invFactor;
        float startYTS = startY * invFactor;
        float endXTS = endX * invFactor;
        float endYTS = endY * invFactor;

        // get parametric from start coord to next point
        float xParam = (nX - startXTS) * factor;
        float yParam = (nY - startYTS) * factor;

        /* For even / odd tri strip rows, triangles are this shape:
        even     odd
        3---2   3---2
        | / |   | \ |
        0---1   0---1
        */

        // Build all 4 positions in normalized cell space, using point-sampled height
        Ogre::Vector3 v0 (startXTS, startYTS, getVertexHeight(land, startX, startY) / 4096.f);
        Ogre::Vector3 v1 (endXTS, startYTS, getVertexHeight(land, endX, startY) / 4096.f);
        Ogre::Vector3 v2 (endXTS, endYTS, getVertexHeight(land, endX, endY) / 4096.f);
        Ogre::Vector3 v3 (startXTS, endYTS, getVertexHeight(land, startX, endY) / 4096.f);
        // define this plane in terrain space
        Ogre::Plane plane;
        // (At the moment, all rows have the same triangle alignment)
        if (true)
        {
            // odd row
            bool secondTri = ((1.0 - yParam) > xParam);
            if (secondTri)
                plane.redefine(v0, v1, v3);
            else
                plane.redefine(v1, v2, v3);
        }
        else
        {
            // even row
            bool secondTri = (yParam > xParam);
            if (secondTri)
                plane.redefine(v0, v2, v3);
            else
                plane.redefine(v0, v1, v2);
        }

        // Solve plane equation for z
        return (-plane.normal.x * nX
                -plane.normal.y * nY
                - plane.d) / plane.normal.z * 4096;

    }

    float Storage::getVertexHeight(const Land *land, int x, int y)
    {
        assert(x < ESM4::Land::VERTS_PER_SIDE);
        assert(y < ESM4::Land::VERTS_PER_SIDE);
        return land->/*getLandData*()->*/mLandData.mHeights[y * ESM4::Land::VERTS_PER_SIDE + x];
    }

    // FIXME: this needs change so that up to 4 textures are combined into one
    //        not sure of the performance impact of doing so - but typically there's only 9 or
    //        so cells loaded at one time so hopefully any impact won't be too noticeable
    //
    //        alternatively, we can try to keep one texture per layer (but scaled and
    //        indexed according to the quad position) - in pathological case that could end up
    //        being 9 x 4 = 36 layers if not combining the textures
    //
    //        Some quick testing suggests that many cells have up to 13 unique textures (on top
    //        of the base textures)
    //
    //        it also shows that the unique textures are not always at the same layer
    //        (e.g. see LAND 0x0000757F, texture 0x0x0004f9e3 are sometimes on layer 0 and
    //        other times on layer 1)
    //
    //        so the choice is between:
    //        a) composite base and up to 13? layers, or
    //        b) composite base and up to 9 composite layers
    Terrain::LayerInfo Storage::getLayerInfo(const std::string& texture)
    {
        // Already have this cached?
        std::map<std::string, Terrain::LayerInfo>::iterator found = mLayerInfoMap.find(texture);
        if (found != mLayerInfoMap.end())
            return found->second;

        Terrain::LayerInfo info;
        info.mParallax = false;
        info.mSpecular = false;
        info.mDiffuseMap = texture;
        std::string texture_ = texture;
        boost::replace_last(texture_, ".", "_nh.");

        if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(texture_))
        {
            info.mNormalMap = texture_;
            info.mParallax = true;
        }
        else
        {
            texture_ = texture;
            boost::replace_last(texture_, ".", "_n.");
            if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(texture_))
                info.mNormalMap = texture_;
        }

        texture_ = texture;
        boost::replace_last(texture_, ".", "_diffusespec.");
        if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(texture_))
        {
            info.mDiffuseMap = texture_;
            info.mSpecular = true;
        }

        // This wasn't cached, so the textures are probably not loaded either.
        // Background load them so they are hopefully already loaded once we need them!
        Ogre::ResourceBackgroundQueue::getSingleton().load("Texture", info.mDiffuseMap, "General");
        if (!info.mNormalMap.empty())
            Ogre::ResourceBackgroundQueue::getSingleton().load("Texture", info.mNormalMap, "General");

        mLayerInfoMap[texture] = info;

        return info;
    }

    // used by QuadTreeNode
    Terrain::LayerInfo Storage::getDefaultLayer()
    {
        Terrain::LayerInfo info;
        info.mDiffuseMap = "textures\\landscape\\terrainhddirt01.dds";
        info.mNormalMap = "textures\\landscape\\terrainhddirt01_n.dds";
        info.mParallax = false;
        info.mSpecular = false;
        info.mIsTes4 = true;

        return info;
    }

    float Storage::getCellWorldSize()
    {
        return static_cast<float>(ESM4::Land::REAL_SIZE);
    }

    // this is used by BufferCache to allocate storage
    // some quads have sizes 17x17 and others 16x16, 16x17, 17x16 but we should use 17x17 and
    // overlap the left and bottom edges
    //
    // WARN: the method name is misleading for TES4
    int Storage::getCellVertices()
    {
        return int(ESM4::Land::VERTS_PER_SIDE / 2) + 1; // int(33 / 2) + 1 = int(16.5) + 1 = 17

        //FIXME: for testing old behaviour
        //return ESM4::Land::VERTS_PER_SIDE;
    }

}
