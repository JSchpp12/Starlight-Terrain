#pragma once

#include <gdal_priv.h>
#include <glm/glm.hpp>
#include <memory>
#include <ogr_spatialref.h>
#include <string>


#include "StarBuffers/Buffer.hpp"
#include "StarMesh.hpp"
#include "device/StarDevice.hpp"

namespace star::terrain
{
class TerrainChunk
{
  public:
    TerrainChunk(const std::string &fullHeightFile, const glm::dvec2 &northEast,
                 const glm::dvec2 &southEast, const glm::dvec2 &southWest, const glm::dvec2 &northWest,
                 const glm::dvec3 &offset, const glm::dvec2 &center);

    /// @brief Load meshes from the provided files
    void load(GDALDataset *sharedDataset);

    star::StarMesh getMesh(star::core::device::DeviceContext &context, std::shared_ptr<star::StarMaterial> myMaterial);

    star::StarBuffers::Buffer &getIndexBuffer()
    {
        assert(this->indBuffer && "Index buffer has not been initialized. Make sure to call load() "
                                  "first.");
        return *this->indBuffer;
    }

    star::StarBuffers::Buffer &getVertexBuffer()
    {
        assert(this->vertBuffer && "Vertex buffer has not been initialized. Make sure to call load() "
                                   "first.");
        return *this->vertBuffer;
    }

    [[nodiscard]] const std::string &getHeightFile() noexcept
    {
        return fullHeightFile;
    }

    static double GetCenterHeightFromGDAL(const std::string &geoTiff);

    static std::optional<double> GetHeightAtLocationFromGDAL(const std::string &path, double latDeg, double lonDeg);

    std::vector<glm::dvec3> lastLine;
    std::vector<glm::dvec3> firstLine;

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

    class TerrainDataset
    {
      public:
        TerrainDataset(GDALDataset *dataset, const glm::dvec2 &northEast, const glm::dvec2 &southEast,
                       const glm::dvec2 &southWest, const glm::dvec2 &northWest, const glm::dvec2 &center,
                       const glm::dvec3 &offset);

        // no copy
        TerrainDataset(const TerrainDataset &) = delete;
        TerrainDataset &operator=(const TerrainDataset &) = delete;

        TerrainDataset(TerrainDataset &&) noexcept = default;
        // no move
        TerrainDataset &operator=(TerrainDataset &&) noexcept = delete;

        ~TerrainDataset();

        float getElevationAtTexCoords(const glm::ivec2 &texCoords) const;

        glm::ivec2 getTexCoordsFromLatLon(const glm::dvec2 &latLon) const;

        glm::ivec2 applyOffsetToTexCoords(const glm::ivec2 &texCoords) const;

        const glm::dvec2 &getNorthEast() const
        {
            return this->m_northEast;
        }
        const glm::dvec2 &getSouthEast() const
        {
            return this->m_southEast;
        }
        const glm::dvec2 &getSouthWest() const
        {
            return this->m_southWest;
        }
        const glm::dvec2 &getNorthWest() const
        {
            return this->m_northWest;
        }
        const glm::dvec3 &getOffset() const
        {
            return this->m_offset;
        }
        const glm::ivec2 &getPixSize() const
        {
            return this->pixSize;
        }
        const glm::dvec2 &getCenter() const
        {
            return this->m_center;
        }

      private:
        glm::ivec2 m_bufferSize{0, 0};
        glm::dvec2 m_northEast, m_southEast, m_southWest, m_northWest, m_center;
        glm::dvec3 m_offset;
        const int pixBorderSize = 2;

        float *gdalBuffer = nullptr;
        glm::ivec2 fullPixSize, maxPixBounds, pixOffset, pixSize;
        double geoTransforms[6];

        void initTransforms(GDALDataset *dataset);

        void initBandSizes(GDALDataset *dataset);

        void initPixelCoords();

        void initGDALBuffer(GDALDataset *dataset);
    };

    std::vector<star::Vertex> verts;
    std::vector<uint32_t> inds;
    std::unique_ptr<star::StarBuffers::Buffer> indBuffer, vertBuffer;
    std::unique_ptr<star::StarMesh> mesh;
    std::string fullHeightFile;
    glm::dvec2 m_northEast, m_southEast, m_southWest, m_northWest, m_center;
    glm::dvec3 m_offset;

    /**
     * @brief Extract height info from the file and calculate ver
     *
     * @param dataset GDALDataset to use
     * @param vertPositions
     * @param vertTextureCoords
     * @param firstLine
     * @param lastLine
     */
    static void loadLocation(TerrainDataset &dataset, std::vector<glm::dvec3> &vertPositions,
                             std::vector<glm::vec2> &vertTextureCoords, std::vector<glm::dvec3> &firstLine,
                             std::vector<glm::dvec3> &lastLine);

    static void loadInds(TerrainDataset &dataset, std::vector<uint32_t> &inds);

    static void calculateNormals(std::vector<star::Vertex> &verts, std::vector<uint32_t> &inds);

    /// @brief Update all vert locations to be centered around the terrain center
    /// @param terrainCenter center of the terrain
    /// @param verts all of the vertices to update
    void centerAroundTerrainOrigin(std::vector<glm::dvec3> &vertPositions, const glm::dvec3 &worldCenterLatLon) const;

    void loadGeomInfo(TerrainDataset &dataset, std::vector<star::Vertex> &verts, std::vector<uint32_t> &inds,
                      std::vector<glm::dvec3> &firstLine, std::vector<glm::dvec3> &lastLine) const;

    static glm::dvec2 calcStep(const glm::dvec2 &startPoint, const glm::dvec2 &horizontalDirection,
                               const double &horizontalStepSize, const glm::dvec2 &verticalDirection,
                               const double &verticalStepSize, const int &stepsX, const int &stepsY);

    static glm::dvec2 calcIntersection(const Line &lineA, const Line &lineB);
};

} // namespace star::terrain
