#include "star_terrain/generated/terrain_chunk/TerrainChunk.hpp"

#include "ManagerRenderResource.hpp"
#include "TransferRequest_IndicesInfo.hpp"
#include "TransferRequest_VertInfo.hpp"
#include "star_terrain/generated/terrain_chunk/TerrainDataset.hpp"
#include "star_terrain/generated/terrain_chunk/TerrainTransform.hpp"
#include "star_terrain/rendering/TerrainVertex.hpp"
#include "star_terrain/util/Distance.hpp"

#include <cmath>
#include <gdal_priv.h>
#include <limits>
#include <ogr_spatialref.h>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>

namespace star::terrain
{

TerrainChunk::TerrainChunk(const std::string &fullHeightFile, const glm::dvec2 &northEast, const glm::dvec2 &southEast,
                           const glm::dvec2 &southWest, const glm::dvec2 &northWest, const glm::dvec3 &offset,
                           const glm::dvec2 &center)
    : fullHeightFile(fullHeightFile), m_northEast(northEast), m_southEast(southEast), m_southWest(southWest),
      m_northWest(northWest), m_offset(offset), m_center(center)
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

ChunkMeshDescription TerrainChunk::createMeshDescription(star::core::device::DeviceContext &context)
{
    const auto &graphicsIndex =
        star::core::helper::GetEngineDefaultQueue(context.getEventBus(), context.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();

    star::Handle vertBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(),
        std::make_unique<star::TransferRequest::VertInfo<rendering::TerrainVertex>>(graphicsIndex, verts));

    star::Handle indBuffer = context.getManagerRenderResource().addRequest(
        context.getDeviceID(), std::make_unique<star::TransferRequest::IndicesInfo>(graphicsIndex, inds));

    glm::vec3 bbMin{std::numeric_limits<float>::max()}, bbMax{std::numeric_limits<float>::lowest()};
    for (const auto &v : verts)
    {
        bbMin = glm::min(bbMin, v.pos);
        bbMax = glm::max(bbMax, v.pos);
    }

    return ChunkMeshDescription{
        bbMin, bbMax, vertBuffer, indBuffer, static_cast<uint32_t>(verts.size()), static_cast<uint32_t>(inds.size())};
}

star::StarMesh TerrainChunk::getMesh(star::core::device::DeviceContext &context,
                                     std::shared_ptr<star::StarMaterial> myMaterial)
{
    const auto desc = createMeshDescription(context);

    return star::StarMesh{desc.vertBuffer,       desc.indBuffer, desc.vertCount, desc.indCount,
                          std::move(myMaterial), desc.bbMin,     desc.bbMax,     false};
}

void TerrainChunk::loadLocation(TerrainDataset &dataset, std::vector<glm::dvec3> &vertPositions,
                                std::vector<glm::vec2> &vertTextureCoords, std::vector<glm::dvec3> &firstLine,
                                std::vector<glm::dvec3> &lastLine)
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

void TerrainChunk::loadInds(TerrainDataset &dataset, std::vector<uint32_t> &inds)
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

void TerrainChunk::calculateNormals(std::vector<rendering::TerrainVertex> &verts, std::vector<uint32_t> &inds)
{
    // calculate normals
    for (int i = 0; i < inds.size(); i += 3)
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

double TerrainChunk::GetHeightAtLocationFromGDAL(const std::string &path, double latDeg, double lonDeg)
{
    GDALAllRegister();

    // RAII-managed dataset so every throw/success path closes it exactly once.
    auto dsDeleter = [](GDALDataset *d) {
        if (d)
            GDALClose(d);
    };
    std::unique_ptr<GDALDataset, decltype(dsDeleter)> ds(
        static_cast<GDALDataset *>(GDALOpen(path.c_str(), GA_ReadOnly)), dsDeleter);

    if (!ds)
    {
        const std::string err = CPLGetLastErrorMsg();
        std::string msg = "Failed to open height raster '" + path + "'";
        if (!err.empty())
            msg += ": " + err;
        STAR_THROW(msg);
    }

    double gt[6];
    if (ds->GetGeoTransform(gt) != CE_None)
        STAR_THROW("Height raster '" + path +
                   "' has no geotransform (unreferenced raster); "
                   "cannot map (lat=" +
                   std::to_string(latDeg) + ", lon=" + std::to_string(lonDeg) + ") to pixels.");

    // Assume north-up, no rotation (gt[2] == 0 and gt[4] == 0).
    if (gt[2] != 0.0 || gt[4] != 0.0)
        STAR_THROW("Height raster '" + path + "' is rotated (gt[2]=" + std::to_string(gt[2]) +
                   ", gt[4]=" + std::to_string(gt[4]) + "); rotated rasters are not supported.");

    GDALRasterBand *band = ds->GetRasterBand(1);
    if (!band)
        STAR_THROW("Height raster '" + path + "' has no band 1.");

    const int nx = band->GetXSize();
    const int ny = band->GetYSize();

    std::unique_ptr<TerrainTransform> transform = TerrainTransform::create(ds.get());
    const OGRSpatialReference *rasterSrs = ds->GetSpatialRef();
    if (rasterSrs && !rasterSrs->IsGeographic() && transform->isNoOp())
    {
        const char *projcs = rasterSrs->GetAttrValue("PROJCS");
        STAR_THROW("Height raster '" + path + "' is projected (CRS: '" + std::string(projcs ? projcs : "unknown") +
                   "') but the 4326->raster coordinate transformation could not be created. "
                   "Ensure the PROJ database is available to GDAL.");
    }

    const glm::dvec2 rasterXY = transform->toRasterXY(latDeg, lonDeg);
    const double px = (rasterXY.x - gt[0]) / gt[1];
    const double py = (rasterXY.y - gt[3]) / gt[5];

    if (std::isnan(px) || std::isnan(py))
        STAR_THROW("Failed to reproject (lat=" + std::to_string(latDeg) + ", lon=" + std::to_string(lonDeg) +
                   ") into the height raster's CRS ('" + path + "').");

    if (px < 0 || py < 0 || px >= nx || py >= ny)
    {
        STAR_THROW("Requested point (lat=" + std::to_string(latDeg) + ", lon=" + std::to_string(lonDeg) +
                   ") maps to pixel (" + std::to_string(px) + ", " + std::to_string(py) +
                   ") which is outside the height raster '" + path + "' (size " + std::to_string(nx) + "x" +
                   std::to_string(ny) +
                   "). Verify the Shape.json center "
                   "lies within the height file's coverage and that the height file's CRS is "
                   "geographic or a supported projection.");
    }

    // Read the nearest pixel. For bilinear, see section 1b below.
    const int ix = static_cast<int>(std::floor(px + 0.5));
    const int iy = static_cast<int>(std::floor(py + 0.5));

    float val = 0.0f;
    if (band->RasterIO(GF_Read, ix, iy, 1, 1, &val, 1, 1, GDT_Float32, 0, 0) != CE_None)
    {
        const std::string err = CPLGetLastErrorMsg();
        std::string msg = "Failed to read elevation pixel from '" + path + "'";
        if (!err.empty())
            msg += ": " + err;
        STAR_THROW(msg);
    }

    int hasNoData = 0;
    const double nodata = band->GetNoDataValue(&hasNoData);
    bool isNoData = (hasNoData && static_cast<double>(val) == nodata);

    double scale = band->GetScale();
    if (scale == 0.0)
        scale = 1.0; // GDAL returns 0 if undefined
    const double offset = band->GetOffset();

    const char *unit = band->GetUnitType(); // typically "m" for meters (can be nullptr)

    return (static_cast<double>(val) * scale + offset);
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

void TerrainChunk::loadGeomInfo(TerrainDataset &dataset, std::vector<rendering::TerrainVertex> &verts,
                                std::vector<uint32_t> &inds, std::vector<glm::dvec3> &firstLine,
                                std::vector<glm::dvec3> &lastLine) const
{
    std::vector<glm::dvec3> rawVertPositionCoords = std::vector<glm::dvec3>();
    std::vector<glm::vec2> vertTextureCoords = std::vector<glm::vec2>();

    loadLocation(dataset, rawVertPositionCoords, vertTextureCoords, firstLine, lastLine);
    loadInds(dataset, inds);
    centerAroundTerrainOrigin(rawVertPositionCoords, dataset.getOffset());

    verts.reserve(rawVertPositionCoords.size());
    for (size_t i = 0; i < rawVertPositionCoords.size(); i++)
    {
        verts.push_back(
            rendering::TerrainVertex{glm::vec3{rawVertPositionCoords.at(i)}, glm::vec3{0.0f}, vertTextureCoords.at(i)});
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

} // namespace star::terrain
