#ifndef NIBTOGRE_BOUNDSFINDER_H
#define NIBTOGRE_BOUNDSFINDER_H

#include <limits>
#include <cmath>

namespace NiBtOgre
{
    // Helper class that computes the bounding box and of a mesh
    class BoundsFinder
    {
        struct MaxMinFinder
        {
            float max, min;

            MaxMinFinder()
            {
                min = std::numeric_limits<float>::infinity();
                max = -min;
            }

            void add(float f)
            {
                if (f > max) max = f;
                if (f < min) min = f;
            }

            // Return Max(max**2, min**2)
            float getMaxSquared()
            {
                float m1 = max*max;
                float m2 = min*min;
                if (m1 >= m2) return m1;
                return m2;
            }
        };

        MaxMinFinder X, Y, Z;

    public:
        // Add 'verts' vertices to the calculation. The 'data' pointer is
        // expected to point to 3*verts floats representing x,y,z for each
        // point.
        void add(float *data, size_t verts)
        {
            for (size_t i=0;i<verts;i++)
            {
                X.add(*(data++));
                Y.add(*(data++));
                Z.add(*(data++));
            }
        }

        // True if this structure has valid values
        bool isValid()
        {
            return
                minX() <= maxX() &&
                minY() <= maxY() &&
                minZ() <= maxZ();
        }

        // Compute radius
        float getRadius()
        {
            assert(isValid());

            // The radius is computed from the origin, not from the geometric
            // center of the mesh.
            return sqrt(X.getMaxSquared() + Y.getMaxSquared() + Z.getMaxSquared());
        }

        float minX() {
            return X.min;
        }
        float maxX() {
            return X.max;
        }
        float minY() {
            return Y.min;
        }
        float maxY() {
            return Y.max;
        }
        float minZ() {
            return Z.min;
        }
        float maxZ() {
            return Z.max;
        }
    };
}

#endif // NIBTOGRE_BOUNDSFINDER_H
