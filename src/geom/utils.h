#ifndef UTILS_H
#define UTILS_H

#include <Eigen/Dense>
#include <array>
#include <fstream>
#include <string>

#include "mcut/mcut.h"

using namespace std;
using namespace Eigen;

inline double dist(const Vector3d& a, const Vector3d& b) {
    return sqrt((a - b).dot(a - b));
}

inline bool same_dir(const Vector3d& a, const Vector3d& b) {
    return a.dot(b) > 0;
}

// Computes the distance between a point and a line segment (i.e. an edge).
double dist_pt2seg(const Vector3d& pt, const array<Vector3d, 2>& line) {
    // Compute direction vector of the line segment
    auto d_vec = (line[1] - line[0]) / dist(line[0], line[1]);

    // Get vector from an endpoint to the point
    auto v = pt - line[0];

    // Compute dot product, then project pt onto the line
    auto t = v.dot(d_vec);
    auto proj = line[0] + t * d_vec;

    return dist(pt, proj);
}

// Computes the distance between a point and a triangle.
double dist_pt2tri(const Vector3d& pt, const array<Vector3d, 3>& tri) {
    // Get triangle normal; this defines a plane
    // 		n.dot(p - p0) = 0
    // 	where n = triangle normal and p0 = anchor point.
    auto r = tri[0], q = tri[1], p = tri[2];
    auto n = (r - p).cross(q - p);

    // Get vector from an anchor point (here, any point on the triangle) to pt
    auto v = pt - tri[0];
    // Then, compute v*n (distance along normal); if in opposite direction, flip n and dist
    auto d = v.dot(n);
    if (d < 0) {
        n *= -1;
    }

    // Subtract triangle normal, scaled by distance, from point
    auto proj = pt - d * n;

    // Check if projected point is within the triangle
    // See https://math.stackexchange.com/a/51328/1178805
    auto AB = tri[1] - tri[0], BC = tri[2] - tri[1], CA = tri[0] - tri[2];
    auto AP = proj - tri[0], BP = proj - tri[1], CP = proj - tri[2];

    // If each cross product is in the same direction as n, we're good
    if (same_dir(AB.cross(AP), n) && same_dir(BC.cross(BP), n) && same_dir(CA.cross(CP), n)) {
        return d;
    } else {
        // If not, compute minimum of distance between point and the 3 edges and 3 verts
        return min(min(min(dist_pt2seg(pt, {tri[0], tri[1]}),   // Compute minimum of distance
                           dist_pt2seg(pt, {tri[1], tri[2]})),  // between pt and the 3 edges
                       dist_pt2seg(pt, {tri[2], tri[0]})),
                   min(min(dist(pt, tri[0]),                    // Compute minimum of distance
                           dist(pt, tri[1])),                   // between pt and the 3 points
                       dist(pt, tri[2])));
    }
}

// Save verts/faces to an .obj filep
void writeOBJ(const std::string& path, const float* ccVertices, const int ccVertexCount,
              const uint32_t* ccFaceIndices, const uint32_t* faceSizes,
              const uint32_t ccFaceCount) {
    printf("write file: %s\n", path.c_str());

    std::ofstream file(path);

    // write vertices and normals
    for (uint32_t i = 0; i < (uint32_t)ccVertexCount; ++i) {
        double x = ccVertices[(McSize)i * 3 + 0];
        double y = ccVertices[(McSize)i * 3 + 1];
        double z = ccVertices[(McSize)i * 3 + 2];
        file << "v " << x << " " << y << " " << z << std::endl;
    }

    int faceVertexOffsetBase = 0;

    // for each face in CC
    for (uint32_t f = 0; f < ccFaceCount; ++f) {
        int faceSize = faceSizes[f];
        file << "f ";
        // for each vertex in face
        for (int v = 0; (v < faceSize); v++) {
            const int ccVertexIdx = ccFaceIndices[(McSize)faceVertexOffsetBase + v];
            file << (ccVertexIdx + 1) << " ";
        }
        file << std::endl;

        faceVertexOffsetBase += faceSize;
    }
}

#endif  // UTILS_H
