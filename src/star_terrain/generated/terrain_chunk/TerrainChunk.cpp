#include "star_terrain/generated/terrain_chunk/TerrainChunk.hpp"

#include "ConfigFile.hpp"
#include "FileHelpers.hpp"
#include "ManagerRenderResource.hpp"
#include "TextureMaterial.hpp"
#include "TransferRequest_IndicesInfo.hpp"
#include "TransferRequest_VertInfo.hpp"
#include "Vertex.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

#include <star_terrain/util/Distance.hpp>

namespace star::terrain
{

TerrainChunk::TerrainChunk(const std::string &fullHeightFile, 
                           const glm::dvec2 &northEast, const glm::dvec2 &southEast, const glm::dvec2 &southWest,
                           const glm::dvec2 &northWest, const glm::dvec3 &offset, const glm::dvec2 &center)
    : fullHeightFile(fullHeightFile), m_northEast(northEast),
      m_southEast(southEast), m_southWest(southWest), m_northWest(northWest), m_offset(offset), m_center(center)
{
}

double TerrainChunk::GetCenterHeightFromGDAL(const std::string &geoTiff)
{
    GDALDataset *dataset = (GDALDataset *)GDALOpen(geoTiff.c_str(), GA_ReadOnly);

    if (dataset == NULL)
    {
        STAR_THROW("Failed to create dataset");
    }

    float *line = nullptr;

    GDALRasterBand *band = dataset->GetRasterBand(1);
    const char *unit = band->GetUnitType();
    double scale = band->GetScale();
    std::cout << "Band unit: " << (unit != nullptr ? unit : "None") << std::endl;

    int nXSize = band->GetXSize();
    int nYSize = band->GetYSize();
    line = (float *)CPLMalloc(sizeof(float) * nXSize * nYSize);
    band->RasterIO(GF_Read, 0, 0, nXSize, nYSize, line, nXSize, nYSize, GDT_Float32, 0, 0);

    double result = line[0];

    CPLFree(line);
    GDALClose(dataset);

    return result;
}

void TerrainChunk::load(GDALDataset *sharedDataset)
{
    assert(sharedDataset != nullptr);

    TerrainDataset dataset =
        TerrainDataset(sharedDataset, m_northEast, m_southEast, m_southWest, m_northWest, m_center, m_offset);

    loadGeomInfo(dataset, verts, inds, this->firstLine, this->lastLine);
}

star::StarMesh TerrainChunk::getMesh(star::core::device::DeviceContext &context,
                                     std::shared_ptr<star::StarMaterial> myMaterial)
{
    const auto &graphicsIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();

    star::Handle vertBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), std::make_unique<star::TransferRequest::VertInfo>(graphicsIndex, verts));

    star::Handle indBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), std::make_unique<star::TransferRequest::IndicesInfo>(graphicsIndex, inds));
    return star::StarMesh{vertBuffer, indBuffer, verts, inds, myMaterial, false};
}

void TerrainChunk::loadLocation(TerrainDataset &dataset, std::vector<glm::dvec3> &vertPositions,
                                std::vector<glm::vec2> &vertTextureCoords, std::vector<glm::dvec3> &firstLine,
                                std::vector<glm::dvec3> &lastLine)
{
    double xTexStep = 1.0f / (double)(dataset.getPixSize().x - 1);
    double yTexStep = 1.0f / (double)(dataset.getPixSize().y - 1);

    const glm::dvec2 horzLine_north = dataset.getNorthEast() - dataset.getNorthWest();
    const glm::dvec2 horzLine_south = dataset.getSouthEast() - dataset.getSouthWest();
    const glm::dvec2 vertLine_west = dataset.getSouthWest() - dataset.getNorthWest();
    const glm::dvec2 vertLine_east = dataset.getSouthEast() - dataset.getNorthEast();

    const double horzStep_north = (glm::length(horzLine_north) / (double)(dataset.getPixSize().x - 1));
    const double horzStep_south = (glm::length(horzLine_south) / (double)(dataset.getPixSize().x - 1));
    const double vertStep_west = (glm::length(vertLine_west) / (double)(dataset.getPixSize().y - 1));
    const double vertStep_east = (glm::length(vertLine_east) / (double)(dataset.getPixSize().y - 1));

    const glm::dvec2 horzLineDir_north = glm::normalize(horzLine_north);
    const glm::dvec2 horzLineDir_south = glm::normalize(horzLine_south);
    const glm::dvec2 vertLineDir_west = glm::normalize(vertLine_west);
    const glm::dvec2 vertLineDir_east = glm::normalize(vertLine_east);

    vertPositions.reserve(dataset.getPixSize().y * dataset.getPixSize().x);

    std::vector<Line> northLines, eastWestLines;
    northLines.reserve(dataset.getPixSize().x);
    for (int i = 0; i < dataset.getPixSize().x; i++)
    {
        const glm::dvec2 bordPosNorth = dataset.getNorthWest() + (horzLineDir_north * horzStep_north * (double)i);
        const glm::dvec2 bordPosSouth = dataset.getSouthWest() + (horzLineDir_south * horzStep_south * (double)i);
        northLines.push_back(Line{bordPosNorth, bordPosSouth});
    }
    eastWestLines.reserve(dataset.getPixSize().y);
    for (int i{0}; i < dataset.getPixSize().y; i++)
    {
        const glm::dvec2 bordPosWest = dataset.getNorthWest() + (vertLineDir_west * vertStep_west * (double)i);
        const glm::dvec2 bordPosEast = dataset.getNorthEast() + (vertLineDir_east * vertStep_east * (double)i);
        eastWestLines.push_back(Line{bordPosWest, bordPosEast});
    }

    vertTextureCoords.reserve(dataset.getPixSize().x * dataset.getPixSize().y);
    // calculate locations
    for (int i = 0; i < dataset.getPixSize().y; i++)
    {
        for (int j = 0; j < dataset.getPixSize().x; j++)
        {
            // find intersection of two
            const glm::dvec2 intersection = calcIntersection(northLines[j], eastWestLines[i]);

            vertPositions.push_back(glm::dvec3{intersection.x, intersection.y,
                                               dataset.getElevationAtTexCoords(dataset.applyOffsetToTexCoords(
                                                   dataset.getTexCoordsFromLatLon(intersection)))});

            vertTextureCoords.push_back(glm::vec2(j * xTexStep, i * yTexStep));
        }
    }
}

void TerrainChunk::loadInds(TerrainDataset &dataset, std::vector<uint32_t> &inds)
{
    uint32_t indexCounter{0};
    inds.reserve(dataset.getPixSize().x * dataset.getPixSize().y * 6);

    for (int i = 0; i < dataset.getPixSize().y; i++)
    {
        for (int j = 0; j < dataset.getPixSize().x; j++)
        {
            if (j % 2 == 1 && i % 2 == 1)
            {
                // this is a 'central' vert where drawing should be based around
                //
                // uppper left
                const uint32_t center = indexCounter;
                const uint32_t centerLeft = indexCounter - 1;
                const uint32_t centerRight = indexCounter + 1;
                const uint32_t upperLeft = indexCounter - 1 - dataset.getPixSize().x;
                const uint32_t upperCenter = indexCounter - dataset.getPixSize().x;
                const uint32_t upperRight = indexCounter - dataset.getPixSize().x + 1;
                const uint32_t lowerLeft = indexCounter + dataset.getPixSize().x - 1;
                const uint32_t lowerCenter = indexCounter + dataset.getPixSize().x;
                const uint32_t lowerRight = indexCounter + dataset.getPixSize().x + 1;
                // 1
                inds.push_back(center);
                inds.push_back(upperLeft);
                inds.push_back(centerLeft);
                // 2
                inds.push_back(center);
                inds.push_back(upperCenter);
                inds.push_back(upperLeft);

                if (i == dataset.getPixSize().y - 1 && j != dataset.getPixSize().x - 1)
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
                else if (i != dataset.getPixSize().y - 1 && j == dataset.getPixSize().x - 1)
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
                else if (i != dataset.getPixSize().y - 1 && j != dataset.getPixSize().x - 1)
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

void TerrainChunk::calculateNormals(std::vector<star::Vertex> &verts, std::vector<uint32_t> &inds)
{
    // calculate normals
    for (int i = 0; i < inds.size(); i += 3)
    {
        auto &vert1 = verts.at(inds.at(i));
        auto &vert2 = verts.at(inds.at(i + 1));
        auto &vert3 = verts.at(inds.at(i + 2));

        glm::vec3 edge1 = vert2.pos - vert1.pos;
        glm::vec3 edge2 = vert3.pos - vert1.pos;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

        vert1.normal += normal;
        vert2.normal += normal;
        vert3.normal += normal;
    }
}

void TerrainChunk::centerAroundTerrainOrigin(std::vector<glm::dvec3> &vertPositions,
                                             const glm::dvec3 &worldCenterLatLon) const
{
    const glm::dvec3 worldCenterECEF =
        star::terrain::util::distance::toECEF(worldCenterLatLon.x, worldCenterLatLon.y, worldCenterLatLon.z);
    const auto worldCenterToENUTransformation =
        star::terrain::util::distance::getECEFToENUTransformation(worldCenterLatLon.x, worldCenterLatLon.y);

    for (int i = 0; i < vertPositions.size(); i++)
    {
        const glm::dvec3 vertECEF =
            star::terrain::util::distance::toECEF(vertPositions[i].x, vertPositions[i].y, vertPositions[i].z);
        const glm::dvec3 displacedECEF = vertECEF - worldCenterECEF;
        const glm::dvec3 result = worldCenterToENUTransformation * displacedECEF;

        vertPositions[i] = glm::vec3{result.y, result.z, result.x};
    }
}

void TerrainChunk::loadGeomInfo(TerrainDataset &dataset, std::vector<star::Vertex> &verts, std::vector<uint32_t> &inds,
                                std::vector<glm::dvec3> &firstLine, std::vector<glm::dvec3> &lastLine) const
{
    std::vector<glm::dvec3> rawVertPositionCoords = std::vector<glm::dvec3>();
    std::vector<glm::vec2> vertTextureCoords = std::vector<glm::vec2>();

    loadLocation(dataset, rawVertPositionCoords, vertTextureCoords, firstLine, lastLine);
    loadInds(dataset, inds);
    centerAroundTerrainOrigin(rawVertPositionCoords, dataset.getOffset());

    verts.reserve(rawVertPositionCoords.size());
    for (size_t i = 0; i < rawVertPositionCoords.size(); i++)
    {
        verts.push_back(star::Vertex(rawVertPositionCoords.at(i), {}, {}, vertTextureCoords.at(i)));
    }

    calculateNormals(verts, inds);
}

glm::dvec2 TerrainChunk::calcStep(const glm::dvec2 &startPoint, const glm::dvec2 &horizontalDirection,
                                  const double &horizontalStepSize, const glm::dvec2 &verticalDirection,
                                  const double &verticalStepSize, const int &stepsX, const int &stepsY)
{
    return glm::dvec2{startPoint + (horizontalDirection * horizontalStepSize * double(stepsX)) +
                      (verticalDirection * verticalStepSize * double(stepsY))};
}

glm::dvec2 TerrainChunk::calcIntersection(const Line &lineA, const Line &lineB)
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

TerrainChunk::TerrainDataset::~TerrainDataset()
{
    try
    {
        if (this->gdalBuffer)
        {
            CPLFree(this->gdalBuffer);
            this->gdalBuffer = nullptr;
        }
    }
    catch (const std::exception &ex)
    {
        STAR_THROW("Memory leak found");
    }
}

TerrainChunk::TerrainDataset::TerrainDataset(GDALDataset *dataset, const glm::dvec2 &northEast,
                                             const glm::dvec2 &southEast, const glm::dvec2 &southWest,
                                             const glm::dvec2 &northWest, const glm::dvec2 &center,
                                             const glm::dvec3 &offset)
    : m_northEast(northEast), m_southEast(southEast), m_southWest(southWest), m_northWest(northWest), m_center(center),
      m_offset(offset)
{
    if (dataset == NULL)
    {
        STAR_THROW("Failed to create dataset");
    }

    initTransforms(dataset);
    initPixelCoords();
    initBandSizes(dataset);
    initGDALBuffer(dataset);
}

std::optional<double> TerrainChunk::GetHeightAtLocationFromGDAL(const std::string &path, double latDeg, double lonDeg)
{
    GDALAllRegister();
    GDALDataset *ds = static_cast<GDALDataset *>(GDALOpen(path.c_str(), GA_ReadOnly));
    if (!ds)
        return std::nullopt;

    double gt[6];
    if (ds->GetGeoTransform(gt) != CE_None)
    {
        GDALClose(ds);
        return std::nullopt;
    }

    // Assume north-up, no rotation:
    // gt[2] (rotation x) == 0 and gt[4] (rotation y) == 0
    // If not north-up, use the general approach in section 2.
    if (gt[2] != 0.0 || gt[4] != 0.0)
    {
        GDALClose(ds);
        return std::nullopt;
    }

    GDALRasterBand *band = ds->GetRasterBand(1);
    if (!band)
    {
        GDALClose(ds);
        return std::nullopt;
    }

    const int nx = band->GetXSize();
    const int ny = band->GetYSize();

    // Convert georef (lon, lat) -> pixel (x, y)
    // Note: gt[5] is typically negative; that's normal.
    const double px = (lonDeg - gt[0]) / gt[1];
    const double py = (latDeg - gt[3]) / gt[5];

    // Clamp or early-exit if outside
    if (px < 0 || py < 0 || px >= nx || py >= ny)
    {
        GDALClose(ds);
        return std::nullopt;
    }

    // Read the nearest pixel. For bilinear, see section 1b below.
    const int ix = static_cast<int>(std::floor(px + 0.5));
    const int iy = static_cast<int>(std::floor(py + 0.5));

    float val = 0.0f;
    if (band->RasterIO(GF_Read, ix, iy, 1, 1, &val, 1, 1, GDT_Float32, 0, 0) != CE_None)
    {
        GDALClose(ds);
        return std::nullopt;
    }

    int hasNoData = 0;
    const double nodata = band->GetNoDataValue(&hasNoData);
    bool isNoData = (hasNoData && static_cast<double>(val) == nodata);

    double scale = band->GetScale();
    if (scale == 0.0)
        scale = 1.0; // GDAL returns 0 if undefined
    const double offset = band->GetOffset();

    const char *unit = band->GetUnitType(); // typically "m" for meters (can be nullptr)

    GDALClose(ds);
    return (static_cast<double>(val) * scale + offset);
}

glm::ivec2 TerrainChunk::TerrainDataset::getTexCoordsFromLatLon(const glm::dvec2 &latLon) const
{
    return glm::ivec2{static_cast<int>(std::round((latLon.y - geoTransforms[0]) / geoTransforms[1])),
                      static_cast<int>(std::round((latLon.x - geoTransforms[3]) / geoTransforms[5]))};
}

glm::ivec2 TerrainChunk::TerrainDataset::applyOffsetToTexCoords(const glm::ivec2 &texCoords) const
{
    return glm::ivec2{texCoords.x - this->pixOffset.x + this->pixBorderSize,
                      texCoords.y - this->pixOffset.y + this->pixBorderSize};
}

float TerrainChunk::TerrainDataset::getElevationAtTexCoords(const glm::ivec2 &texCoords) const
{
    const int safeX = std::clamp(texCoords.x, 0, this->m_bufferSize.x - 1);
    const int safeY = std::clamp(texCoords.y, 0, this->m_bufferSize.y - 1);

    return this->gdalBuffer[safeY * this->m_bufferSize.x + safeX];
}

void TerrainChunk::TerrainDataset::initTransforms(GDALDataset *dataset)
{
    if (GDALGetGeoTransform(dataset, this->geoTransforms) != CPLE_None)
        STAR_THROW("Failed to obtain proper geotransform");
}

void TerrainChunk::TerrainDataset::initPixelCoords()
{
    const glm::ivec2 tNorthEast = getTexCoordsFromLatLon(m_northEast);
    const glm::ivec2 tNorthWest = getTexCoordsFromLatLon(m_northWest);
    const glm::ivec2 tSouthEast = getTexCoordsFromLatLon(m_southEast);
    const glm::ivec2 tSouthWest = getTexCoordsFromLatLon(m_southWest);

    const auto crossA = glm::ivec2{std::abs(tSouthEast.x - tNorthWest.x), std::abs(tSouthEast.y - tNorthWest.y)};
    const auto crossB = glm::ivec2{std::abs(tSouthWest.x - tNorthEast.x), std::abs(tSouthWest.y - tNorthEast.y)};
    this->pixSize = glm::ivec2{std::floor((crossA.x + crossB.x) / 2), std::floor((crossA.y + crossB.y) / 2)};

    // Partial edge chunks can be narrower/shorter than a single heightmap
    // pixel. Rounding their corner pixel coordinates then collapses to the
    // same value, yielding a zero-size dimension and a mesh with zero
    // vertices. Clamp each dimension to a minimum of 2 so sub-pixel slivers
    // still generate a valid minimal mesh and the pixSize - 1 divisions
    // in loadLocation() stay safe.
    constexpr int minPixDim = 2;
    this->pixSize.x = std::max(this->pixSize.x, minPixDim);
    this->pixSize.y = std::max(this->pixSize.y, minPixDim);

    this->pixOffset = tNorthWest;
    this->maxPixBounds = this->pixSize + this->pixOffset;
}

void TerrainChunk::TerrainDataset::initBandSizes(GDALDataset *dataset)
{
    GDALRasterBand *band = dataset->GetRasterBand(1);
    this->fullPixSize = glm::ivec2{band->GetXSize(), band->GetYSize()};
}

void TerrainChunk::TerrainDataset::initGDALBuffer(GDALDataset *dataset)
{
    GDALRasterBand *band = dataset->GetRasterBand(1);

    int xOff = this->pixOffset.x - this->pixBorderSize;
    int yOff = this->pixOffset.y - this->pixBorderSize;
    int xSize = this->pixSize.x + (2 * this->pixBorderSize);
    int ySize = this->pixSize.y + (2 * this->pixBorderSize);

    xOff = std::max(0, xOff);
    yOff = std::max(0, yOff);
    xSize = std::min(xSize, this->fullPixSize.x - xOff);
    ySize = std::min(ySize, this->fullPixSize.y - yOff);

    // Store the actual dimensions so other methods index correctly
    this->m_bufferSize = glm::ivec2{xSize, ySize};

    this->gdalBuffer = (float *)CPLMalloc(sizeof(float) * xSize * ySize);

    CPLErr error = band->RasterIO(GF_Read, xOff, yOff, xSize, ySize, this->gdalBuffer, xSize, ySize, GDT_Float32, 0, 0);

    if (error != CE_None)
    {
        STAR_THROW("Failed to read raster band");
    }
}
} // namespace star::terrain
