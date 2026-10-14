#pragma once

#include "star_terrain/io/TargetChunkInfo.hpp"
#include "star_terrain/rendering/geometry/FullTerrainVertex.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

class GDALDataset;

namespace star::terrain
{
class TerrainDataset;

/// CPU-side geometry loaded for a single terrain chunk. Vertices are centered around the terrain origin (local ENU
/// space) and the bounding box is computed by the loader.
struct LoadedGeometry
{
    std::vector<rendering::geometry::FullTerrainVertex> verts;
    std::vector<uint32_t> inds;
    glm::vec3 bbMin{0.0f};
    glm::vec3 bbMax{0.0f};
};

/// @breif Loading policy for the full-geometry terrain shape representation.
class LoadFullGeometry
{
  public:
    using Payload = LoadedGeometry;

    LoadFullGeometry(TargetChunkInfo chunkInfo, GDALDataset *sharedDataset);

    /// @brief Load the geometry for this chunk from the provided dataset
    LoadedGeometry operator()() const;

  private:
    struct Line
    {
        // A line is stored by its two endpoint positions (x = lat, y = lon).
        // Storing the endpoints (instead of a slope/intercept representation
        // lon = slope*lat + intercept) keeps axis-aligned chunk edges
        // representable: east-west edges sit at a constant latitude, which
        // would otherwise give an infinite slope and break intersection.
        const glm::dvec2 pointA, pointB;

        Line(const glm::dvec2 &a, const glm::dvec2 &b) : pointA(a), pointB(b)
        {
        }
    };

    TargetChunkInfo m_chunkInfo;
    GDALDataset *m_sharedDataset = nullptr;

    /**
     * @brief Extract height info from the file and calculate vertices
     *
     * @param dataset GDALDataset to use
     * @param vertPositions
     * @param vertTextureCoords
     */
    static void loadLocation(TerrainDataset &dataset, std::vector<glm::dvec3> &vertPositions,
                             std::vector<glm::vec2> &vertTextureCoords);

    static void loadInds(TerrainDataset &dataset, std::vector<uint32_t> &inds);

    static void calculateNormals(std::vector<rendering::geometry::FullTerrainVertex> &verts,
                                 std::vector<uint32_t> &inds);

    /// @brief Update all vert locations to be centered around the terrain center
    /// @param terrainCenter center of the terrain
    /// @param verts all of the vertices to update
    static void centerAroundTerrainOrigin(std::vector<glm::dvec3> &vertPositions, const glm::dvec3 &worldCenterLatLon);

    static LoadedGeometry loadGeomInfo(TerrainDataset &dataset);

    static glm::dvec2 calcIntersection(const Line &lineA, const Line &lineB);
};

} // namespace star::terrain