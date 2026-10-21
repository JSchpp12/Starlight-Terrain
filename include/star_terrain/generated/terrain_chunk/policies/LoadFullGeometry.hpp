#pragma once

#include "star_terrain/io/TargetChunkInfo.hpp"
#include "star_terrain/rendering/TerrainRenderingType.hpp"
#include "star_terrain/rendering/geometry/FullTerrainVertex.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <vector>

class GDALDataset;

namespace star::terrain
{
class TerrainDataset;

/// @brief CPU-side geometry loaded for a single terrain chunk. Vertices are centered around the terrain origin
/// (localspace) and the bounding box is computed by the loader. ENU
struct LoadedGeometry
{
    std::vector<rendering::geometry::FullTerrainVertex> verts;
    std::vector<uint32_t> inds;
    glm::vec3 bbMin{0.0f};
    glm::vec3 bbMax{0.0f};
};

/// @brief Loading policy for the full-geometry terrain shape representation.
class LoadFullGeometry
{
  public:
    using Payload = LoadedGeometry;

    LoadFullGeometry(TargetChunkInfo chunkInfo, rendering::ElevationType elevationType, GDALDataset *sharedDataset);

    /// @brief Load the geometry for this chunk from the provided dataset
    LoadedGeometry operator()() const;

  private:
    struct Line
    {
        /// @brief A line is stored by its two endpoint positions (x = lat, y = lon). representable: east-west edges sit
        /// at a constant latitude, which would otherwise give an infinite slope and break intersection.
        glm::dvec2 pointA, pointB;

        Line(glm::dvec2 a, glm::dvec2 b) : pointA(std::move(a)), pointB(std::move(b))
        {
        }
    };

    TargetChunkInfo m_chunkInfo;
    rendering::ElevationType m_elevationType;
    GDALDataset *m_sharedDataset{nullptr};

    /**
     * @brief Extract height info from the file and calculate vertices
     *
     * @param dataset GDALDataset to use
     * @param vertPositions Vector to fill with vertex positions
     * @param vertTextureCoords Vector to fill with texture coordinates for each vertex
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

    /// @brief Flatten the terrain to a plane at height 0, keeping the vertex
    /// positions in the local (X = north, Y = up, Z = east) frame.
    /// The local Y axis is the terrain's height axis, so every vertex keeps its
    /// horizontal placement and loses only its elevation.
    /// @param vertPositions all of the vertices to flatten
    static void flattenToHeightZero(std::vector<glm::dvec3> &vertPositions);

    static LoadedGeometry loadGeomInfo(TerrainDataset &dataset, rendering::ElevationType elevationType);

    static glm::dvec2 calcIntersection(const Line &lineA, const Line &lineB);
};

} // namespace star::terrain