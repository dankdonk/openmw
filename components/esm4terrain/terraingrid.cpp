/*
 * Copyright (c) 2015 scrawl <scrawl@baseoftrash.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "terraingrid.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreAxisAlignedBox.h>
#include <OgreTextureManager.h>

#include <components/terrain/chunk.hpp>
//#include "storage.hpp"

namespace ESM4Terrain
{

TerrainGrid::TerrainGrid(Ogre::SceneManager *sceneMgr,
    Terrain::Storage *storage, int visibilityFlags, bool shaders, Terrain::Alignment align, ESM4::FormId world)
    : Terrain::TerrainGrid(sceneMgr, storage, visibilityFlags, shaders, align)//, mWorld(world)
{
}

TerrainGrid::~TerrainGrid()
{
}

#if 0
void TerrainGrid::update(const Ogre::Vector3 &cameraPos)
{
}

// Chunk is Ogre::Renderable and Ogre::MovableObject
// mChunk is attached to mSceneNode then its material updated
//
// struct GridElement
// {
//     Ogre::SceneNode* mSceneNode;
//     Terrain::MaterialGenerator mMaterialGenerator;
//     Terrain::Chunk* mChunk;
// };
//
// typedef std::map<std::pair<int, int>, std::vector<Terrain::GridElement> > TES4Grid;
//
// According to Ogre 1.9 API, a Renderable class must follow this:
//
//   Classes implementing this interface must be based on a single material, a single world
//   matrix (or a collection of world matrices which are blended by weights), and must be
//   renderable via a single render operation.
//
// That seems to imply that there can be only one material for a Chunk, which leads to having a
// chunk per quadrant of a cell for TES4 where each quadrant can have several different layers
// of textures blended in with different opacity levels.



// Q how to apply textures to only certain verticies?


// Not sure if a cell should have 4 chunks or 1
// If 4 chunks, need to split the cell and load the vertex, normals and colours 4 times
// If 1 chunk, not sure how to load the textures, and whether any future processing will affect it
void TerrainGrid::loadCell(int x, int y)
{
    Tes4Grid::iterator it = mGrid.find(std::make_pair(x, y);
    if (it != mGrid.end())
        return; // already loaded

    std::vector<Terrain::GridElement> elements;
    elements.resize(4);

    // FIXME: how to get TES4 data without changing the interface?  mStorage is a pointer
    // to the base class, so may need to resort to down-casting
    ESM4Terrain::Storage* storage = static_cast<ESM4Terrain::Storage*>(mStorage);

    // there are 4 quadrants: 0 = bottom left. 1 = bottom right. 2 = upper-left. 3 = upper-right
    for (int i = 0; i < 4; ++i)
    {
        // (y+1)*4096 +---------------+
        //            |               |
        //        .   |               |
        //        .   |               |
        //        .   |     cell      |
        //        .   |    (x, y)     |
        //            |               |
        //            |               |
        //     y*4096 +---------------+
        //          x*4096  .....  (x+1)*4096

        // FIXME: ? why add 0.5f here?
        //
        // But noticed that Storage::getminmaxHeights() and Storage::fillVertexBuffers()
        // subtract Vector2(size/2.f, size2/f) to get origin, where size is 1 when called from
        // TerrainGrid::loadCell()
        Ogre::Vector2 center(x+0.5f, y+0.5f);
        float minH, maxH;
        if (!mStorage->getMinMaxQuadHeights(1, center, minH, maxH, i))
            return; // no terrain defined

        // FIXME: below should be different for each quadrant
        Ogre::Vector3 min (-0.5f*mStorage->getCellWorldSize(),
                           -0.5f*mStorage->getCellWorldSize(),
                           minH);
        Ogre::Vector3 max (0.5f*mStorage->getCellWorldSize(),
                           0.5f*mStorage->getCellWorldSize(),
                           maxH);

        Ogre::AxisAlignedBox bounds(min, max);

        Terrain::GridElement element;

        Ogre::Vector2 worldCenter = center*mStorage->getCellWorldSize();
        // ---------




        element.mSceneNode = mRootNode->createChildSceneNode(Ogre::Vector3(worldCenter.x, worldCenter.y, 0));

        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<Ogre::uint8> colours;
        mStorage->fillVertexBuffers(0, 1, center, mAlign, positions, normals, colours);

        element.mChunk = new Terrain::Chunk(mCache.getUVBuffer(), bounds, positions, normals, colours);
        element.mChunk->setIndexBuffer(mCache.getIndexBuffer(0));
        element.mChunk->setVisibilityFlags(mVisibilityFlags);
        element.mChunk->setCastShadows(true);

        std::vector<Ogre::PixelBox> blendmaps;
        std::vector<Terrain::LayerInfo> layerList;
        mStorage->getBlendmaps(1, center, mShaders, blendmaps, layerList);

        element.mMaterialGenerator.setLayerList(layerList);

        // upload blendmaps to GPU
        std::vector<Ogre::TexturePtr> blendTextures;
        for (std::vector<Ogre::PixelBox>::const_iterator it = blendmaps.begin(); it != blendmaps.end(); ++it)
        {
            static int count=0;
            Ogre::TexturePtr map = Ogre::TextureManager::getSingleton().createManual("terrain/blend/"
                + Ogre::StringConverter::toString(count++), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                Ogre::TEX_TYPE_2D, it->getWidth(), it->getHeight(), 0, it->format);

            Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(it->data, it->getWidth()*it->getHeight()*Ogre::PixelUtil::getNumElemBytes(it->format), true));
            map->loadRawData(stream, it->getWidth(), it->getHeight(), it->format);
            blendTextures.push_back(map);
        }

        element.mMaterialGenerator.setBlendmapList(blendTextures);

        element.mSceneNode->attachObject(element.mChunk);
        updateMaterial(element);

        elements[i] = element;
        //mGrid[std::make_pair(x,y)] = element;
    }
    mGrid.insert(std::make_pair(std::make_pair(x,y), elements));
}

void TerrainGrid::unloadCell(int x, int y)
{
    Grid::iterator it = mGrid.find(std::make_pair(x,y));
    if (it == mGrid.end())
        return;

    GridElement& element = it->second;
    delete element.mChunk;
    element.mChunk = NULL;

    const std::vector<Ogre::TexturePtr>& blendmaps = element.mMaterialGenerator.getBlendmapList();
    for (std::vector<Ogre::TexturePtr>::const_iterator it = blendmaps.begin(); it != blendmaps.end(); ++it)
        Ogre::TextureManager::getSingleton().remove((*it)->getName());

    mSceneMgr->destroySceneNode(element.mSceneNode);
    element.mSceneNode = NULL;

    mGrid.erase(it);
}

void TerrainGrid::updateMaterial(GridElement &element)
{
    element.mMaterialGenerator.enableShadows(getShadowsEnabled());
    element.mMaterialGenerator.enableSplitShadows(getSplitShadowsEnabled());
    element.mChunk->setMaterial(element.mMaterialGenerator.generate());
}

void TerrainGrid::applyMaterials(bool shadows, bool splitShadows)
{
    mShadows = shadows;
    mSplitShadows = splitShadows;
    for (Grid::iterator it = mGrid.begin(); it != mGrid.end(); ++it)
    {
        updateMaterial(it->second);
    }
}

bool TerrainGrid::getVisible()
{
    return mVisible;
}

void TerrainGrid::setVisible(bool visible)
{
    mVisible = visible;
    mRootNode->setVisible(visible);
}

Ogre::AxisAlignedBox TerrainGrid::getWorldBoundingBox (const Ogre::Vector2& center)
{
    int cellX = static_cast<int>(std::floor(center.x));
    int cellY = static_cast<int>(std::floor(center.y));

    Grid::iterator it = mGrid.find(std::make_pair(cellX, cellY));
    if (it == mGrid.end())
        return Ogre::AxisAlignedBox::BOX_NULL;

    Terrain::Chunk* chunk = it->second.mChunk;
    Ogre::SceneNode* node = it->second.mSceneNode;
    Ogre::AxisAlignedBox box = chunk->getBoundingBox();
    box = Ogre::AxisAlignedBox(box.getMinimum() + node->getPosition(), box.getMaximum() + node->getPosition());
    return box;
}

void TerrainGrid::syncLoad()
{

}
#endif
}
