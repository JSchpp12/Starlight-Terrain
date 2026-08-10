#include "star_terrain/generated/terrain_chunk/TerrainTransform.hpp"

namespace star::terrain
{
std::unique_ptr<TerrainTransform> TerrainTransform::create(GDALDataset *ds) noexcept
{
    OGRCoordinateTransformation *ct = nullptr;

    if (ds)
    {
        const OGRSpatialReference *rasterSrc = ds->GetSpatialRef();
        if (rasterSrc && !rasterSrc->IsGeographic())
        {
            OGRSpatialReference geoSrs;
            geoSrs.importFromEPSG(4326);
#if GDAL_VERSION_MAJOR >= 3
            geoSrs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER); // x=lon, y=lat
#endif

            OGRSpatialReference rasterSrs;
            rasterSrs = *rasterSrc; // copy the raster CRS
#if GDAL_VERSION_MAJOR >= 3
            rasterSrs.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER); // x=easting, y=northing
#endif

            ct = OGRCreateCoordinateTransformation(&geoSrs, &rasterSrs);
            // If creation fails (e.g. PROJ unavailable) ct stays null and the
            // transform behaves as a no-op; the caller's pixel math then lands
            // out-of-bounds and surfaces a descriptive error instead of crashing.
        }
    }

    return std::unique_ptr<TerrainTransform>(new TerrainTransform(ct));
}

glm::dvec2 TerrainTransform::toRasterXY(double lat, double lon) const noexcept
{
    if (!m_ct)
        return {lon, lat}; // legacy: x=lon, y=lat
    double x = lon, y = lat;
    if (!m_ct->Transform(1, &x, &y))
        return {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()};
    return {x, y};
}

TerrainTransform::TerrainTransform(OGRCoordinateTransformation *ct) noexcept : m_ct(ct)
{
}

TerrainTransform::~TerrainTransform()
{
    if (m_ct)
        OGRCoordinateTransformation::DestroyCT(m_ct);
}

} // namespace star::terrain