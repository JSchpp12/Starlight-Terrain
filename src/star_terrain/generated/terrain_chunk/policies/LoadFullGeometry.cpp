#include "star_terrain/generated/terrain_chunk/policies/LoadFullGeometry.hpp"

#include "star_terrain/generated/terrain_chunk/TerrainDataset.hpp"
#include "star_terrain/rendering/geometry/FullTerrainVertex.hpp"
#include "star_terrain/util/Distance.hpp"

#include <gdal_priv.h>

#include <cassert>
#include <cmath>
#include <limits>
#include <utility>

namespace star::terrain
{

LoadFullGeometry::LoadFullGeometry(TargetChunkInfo chunkInfo, GDALDataset *sharedDataset)
    : m_chunkInfo(std::move(chunkInfo)), m_sharedDataset(sharedDataset)
{
}

LoadedGeometry LoadFullGeometry::operator()() const
{
    assert(m_sharedDataset != nullptr);

    TerrainDataset dataset =
        TerrainDataset(m_sharedDataset, m_chunkInfo.northEast, m_chunkInfo.southEast, m_chunkInfo.southWest,
                       m_chunkInfo.northWest, m_chunkInfo.center, m_chunkInfo.offset);

    LoadedGeometry loaded = loadGeomInfo(dataset);

    // Compute the bounds here (worker thread) so the upload step stays pure GPU work.
    loaded.bbMin = glm::vec3{std::numeric_limits<float>::max()};
    loaded.bbMax = glm::vec3{std::numeric_limits<float>::lowest()};
    for (const auto &vert : loaded.verts)
    {
        loaded.bbMin = glm::min(loaded.bbMin, vert.pos);
        loaded.bbMax = glm::max(loaded.bbMax, vert.pos);
    }

    return loaded;
}

void LoadFullGeometry::loadLocation(TerrainDataset &dataset, std::vector<glm::dvec3> &vertPositions,
                                    std::vector<glm::vec2> &vertTextureCoords)
{
    const auto pixSize = dataset.getPixSize();
    const int nx = pixSize.x;
    const int ny = pixSize.y;
    const size_t numVerts = static_cast<size_t>(nx) * static_cast<size_t>(ny);

    double xTexStep = 1.0f / (double)(nx - 1);
    double yTexStep = 1.0f / (double)(ny - 1);

    const glm::dvec2 horzLine_north = dataset.getNorthEast() - dataset.getNorthWest();
    const glm::dvec2 horzLine_south = dataset.getSouthEast() - dataset.getSouthWest();
    const glm::dvec2 vertLine_west = dataset.getSouthWest() - dataset.getNorthWest();
    const glm::dvec2 vertLine_east = dataset.getSouthEast() - dataset.getNorthEast();

    const double horzStep_north = (glm::length(horzLine_north) / (double)(nx - 1));
    const double horzStep_south = (glm::length(horzLine_south) / (double)(nx - 1));
    const double vertStep_west = (glm::length(vertLine_west) / (double)(ny - 1));
    const double vertStep_east = (glm::length(vertLine_east) / (double)(ny - 1));

    const glm::dvec2 horzLineDir_north = glm::normalize(horzLine_north);
    const glm::dvec2 horzLineDir_south = glm::normalize(horzLine_south);
    const glm::dvec2 vertLineDir_west = glm::normalize(vertLine_west);
    const glm::dvec2 vertLineDir_east = glm::normalize(vertLine_east);

    std::vector<Line> northLines, eastWestLines;
    northLines.reserve(nx);
    for (int i = 0; i < nx; i++)
    {
        const glm::dvec2 bordPosNorth = dataset.getNorthWest() + (horzLineDir_north * horzStep_north * (double)i);
        const glm::dvec2 bordPosSouth = dataset.getSouthWest() + (horzLineDir_south * horzStep_south * (double)i);
        northLines.push_back(Line{bordPosNorth, bordPosSouth});
    }
    eastWestLines.reserve(ny);
    for (int i{0}; i < ny; i++)
    {
        const glm::dvec2 bordPosWest = dataset.getNorthWest() + (vertLineDir_west * vertStep_west * (double)i);
        const glm::dvec2 bordPosEast = dataset.getNorthEast() + (vertLineDir_east * vertStep_east * (double)i);
        eastWestLines.push_back(Line{bordPosWest, bordPosEast});
    }

    // Pre-size output vectors -- resize+index avoids the per-element capacity
    // checks and iterator validation that push_back incurs in debug builds.
    vertPositions.resize(numVerts);
    vertTextureCoords.resize(numVerts);

    // collect all intersection lat/lon + texture coords
    std::vector<double> latGeo(numVerts), lonGeo(numVerts);
    for (int i = 0; i < ny; i++)
    {
        for (int j = 0; j < nx; j++)
        {
            const size_t k = static_cast<size_t>(i) * nx + j;
            const glm::dvec2 intersection = calcIntersection(northLines[j], eastWestLines[i]);
            latGeo[k] = intersection.x;
            lonGeo[k] = intersection.y;
            vertTextureCoords[k] = glm::vec2(j * xTexStep, i * yTexStep);
        }
    }

    // batch reprojection
    std::vector<float> elevations = dataset.reprojectBatch(latGeo.data(), lonGeo.data(), numVerts);

    // assemble
    for (size_t k = 0; k < numVerts; k++)
    {
        vertPositions[k] = glm::dvec3{latGeo[k], lonGeo[k], elevations[k]};
    }
}

void LoadFullGeometry::loadInds(TerrainDataset &dataset, std::vector<uint32_t> &inds)
{
    const auto pixSize = dataset.getPixSize();
    const int nx = pixSize.x;
    const int ny = pixSize.y;
    const uint32_t rowWidth = static_cast<uint32_t>(nx);
    uint32_t indexCounter{0};
    inds.reserve(static_cast<size_t>(nx) * ny * 6);

    for (int i = 0; i < ny; i++)
    {
        for (int j = 0; j < nx; j++)
        {
            if (j % 2 == 1 && i % 2 == 1)
            {
                // this is a 'central' vert where drawing should be based around
                //
                // uppper left
                const uint32_t center = indexCounter;
                const uint32_t centerLeft = indexCounter - 1;
                const uint32_t centerRight = indexCounter + 1;
                const uint32_t upperLeft = indexCounter - 1 - rowWidth;
                const uint32_t upperCenter = indexCounter - rowWidth;
                const uint32_t upperRight = indexCounter - rowWidth + 1;
                const uint32_t lowerLeft = indexCounter + rowWidth - 1;
                const uint32_t lowerCenter = indexCounter + rowWidth;
                const uint32_t lowerRight = indexCounter + rowWidth + 1;
                // 1
                inds.push_back(center);
                inds.push_back(upperLeft);
                inds.push_back(centerLeft);
                // 2
                inds.push_back(center);
                inds.push_back(upperCenter);
                inds.push_back(upperLeft);

                if (i == ny - 1 && j != nx - 1)
                {
                    // bottom piece
                    // can do 3,4,5,6,
                    // 7
                    inds.push_back(center);
                    inds.push_back(centerRight);
                    inds.push_back(upperRight);
                    // 8
                    inds.push_back(center);
                    inds.push_back(upperRight);
                    inds.push_back(upperCenter);
                }
                else if (i != ny - 1 && j == nx - 1)
                {
                    // side piece
                    // cant do 5,6,7,8
                    // 3
                    inds.push_back(center);
                    inds.push_back(centerLeft);
                    inds.push_back(lowerLeft);
                    // 4
                    inds.push_back(center);
                    inds.push_back(lowerLeft);
                    inds.push_back(lowerCenter);
                }
                else if (i != ny - 1 && j != nx - 1)
                {
                    // 3
                    inds.push_back(center);
                    inds.push_back(upperRight);
                    inds.push_back(upperCenter);
                    // 7
                    inds.push_back(center);
                    inds.push_back(centerRight);
                    inds.push_back(upperRight);
                    // 5
                    inds.push_back(center);
                    inds.push_back(lowerRight);
                    inds.push_back(centerRight);
                    // 6
                    inds.push_back(center);
                    inds.push_back(lowerCenter);
                    inds.push_back(lowerRight);
                    // 7
                    inds.push_back(center);
                    inds.push_back(lowerLeft);
                    inds.push_back(lowerCenter);
                    // 8
                    inds.push_back(center);
                    inds.push_back(centerLeft);
                    inds.push_back(lowerLeft);
                }
            }
            indexCounter++;
        }
    }
}

void LoadFullGeometry::calculateNormals(std::vector<rendering::geometry::FullTerrainVertex> &verts,
                                        std::vector<uint32_t> &inds)
{
    // calculate normals
    for (size_t i = 0; i < inds.size(); i += 3)
    {
        auto &vert1 = verts[inds[i]];
        auto &vert2 = verts[inds[i + 1]];
        auto &vert3 = verts[inds[i + 2]];

        glm::vec3 edge1 = vert2.pos - vert1.pos;
        glm::vec3 edge2 = vert3.pos - vert1.pos;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        vert1.normal += normal;
        vert2.normal += normal;
        vert3.normal += normal;
    }
}

void LoadFullGeometry::centerAroundTerrainOrigin(std::vector<glm::dvec3> &vertPositions,
                                                 const glm::dvec3 &worldCenterLatLon)
{
    const glm::dvec3 worldCenterECEF =
        star::terrain::util::distance::toECEF(worldCenterLatLon.x, worldCenterLatLon.y, worldCenterLatLon.z);
    const auto worldCenterToENUTransformation =
        star::terrain::util::distance::getECEFToENUTransformation(worldCenterLatLon.x, worldCenterLatLon.y);

    for (size_t i = 0; i < vertPositions.size(); i++)
    {
        const glm::dvec3 vertECEF =
            star::terrain::util::distance::toECEF(vertPositions[i].x, vertPositions[i].y, vertPositions[i].z);
        const glm::dvec3 displacedECEF = vertECEF - worldCenterECEF;
        const glm::dvec3 result = worldCenterToENUTransformation * displacedECEF;

        vertPositions[i] = glm::vec3{result.y, result.z, result.x};
    }
}

LoadedGeometry LoadFullGeometry::loadGeomInfo(TerrainDataset &dataset)
{
    std::vector<glm::dvec3> rawVertPositionCoords = std::vector<glm::dvec3>();
    std::vector<glm::vec2> vertTextureCoords = std::vector<glm::vec2>();

    LoadedGeometry loaded;
    loadLocation(dataset, rawVertPositionCoords, vertTextureCoords);
    loadInds(dataset, loaded.inds);
    centerAroundTerrainOrigin(rawVertPositionCoords, dataset.getOffset());

    loaded.verts.reserve(rawVertPositionCoords.size());
    for (size_t i = 0; i < rawVertPositionCoords.size(); i++)
    {
        loaded.verts.push_back(rendering::geometry::FullTerrainVertex{glm::vec3{rawVertPositionCoords.at(i)},
                                                                      glm::vec3{0.0f}, vertTextureCoords.at(i)});
    }

    calculateNormals(loaded.verts, loaded.inds);

    return loaded;
}

glm::dvec2 LoadFullGeometry::calcIntersection(const Line &lineA, const Line &lineB)
{
    // formula handles vertical/horizontal edges without degeneracy.
    const glm::dvec2 &p1 = lineA.pointA;
    const glm::dvec2 &p2 = lineA.pointB;
    const glm::dvec2 &p3 = lineB.pointA;
    const glm::dvec2 &p4 = lineB.pointB;

    const double denom = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    assert(std::abs(denom) > 0.0 && "Attempted to find intersection of parallel lines");

    const double px =
        ((p1.x * p2.y - p1.y * p2.x) * (p3.x - p4.x) - (p1.x - p2.x) * (p3.x * p4.y - p3.y * p4.x)) / denom;
    const double py =
        ((p1.x * p2.y - p1.y * p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x * p4.y - p3.y * p4.x)) / denom;

    return glm::dvec2{px, py};
}

} // namespace star::terrain