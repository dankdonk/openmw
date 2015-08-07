/*
  Copyright (C) 2015 cc9cii

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

*/
#ifndef ESM4_NAVI_H
#define ESM4_NAVI_H

#include <vector>
#include <map>

#include "common.hpp" // CellGrid, Vertex

namespace ESM4
{
    class Reader;
    //class Writer;

    struct Navigation
    {
#pragma pack(push,1)
        struct DoorRef
        {
            std::uint32_t unknown;
            std::uint32_t formId;
        };

        struct Triangle
        {
            std::uint16_t vertexIndex0;
            std::uint16_t vertexIndex1;
            std::uint16_t vertexIndex2;
        };
#pragma pack(pop)

        struct IslandInfo
        {
            float minX;
            float minY;
            float minZ;
            float maxX;
            float maxY;
            float maxZ;
            std::vector<Triangle> triangles;
            std::vector<Vertex> verticies;

            void load(ESM4::Reader& reader);
        };

        struct NavMeshInfo
        {
            std::uint32_t formId;
            std::uint32_t flags;
            // center point of the navmesh
            float x;
            float y;
            float z;
            std::uint32_t flagPrefMerges;
            std::vector<std::uint32_t> formIdMerged;
            std::vector<std::uint32_t> formIdPrefMerged;
            std::vector<DoorRef> linkedDoors;
            std::vector<IslandInfo> islandInfo;
            std::uint32_t locationMarker;
            std::uint32_t worldSpaceId;
            CellGrid cellGrid;

            void load(ESM4::Reader& reader);
        };

        std::string mEditorId;

        std::vector<NavMeshInfo> mNavMeshInfo;

        std::vector<std::pair<std::uint32_t, std::vector<std::uint32_t> > > mPreferredPaths;

        std::map<std::uint32_t, std::uint32_t> mPathIndexMap;

        Navigation();
        ~Navigation();

        void load(ESM4::Reader& reader);
        //void save(ESM4::Writer& writer) const;

    //private:

        //Navigation(const Navigation& other);
        //Navigation& operator=(const Navigation& other); // FIXME required by Collection<T>

    };
}

#endif // ESM4_NAVI_H
