#pragma once

#include <expected>
#include <filesystem>
#include <string>

class GDALDataset;

namespace star::terrain
{
/// Error produced when a GDAL dataset cannot be loaded.
struct GDALLoadError
{
    enum class Cause
    {
        dataset_open_failed
    };

    Cause cause;
    std::string detail;
};

/// Acquires a read-only GDAL dataset for a height raster.
class GDALDatasetLoader
{
  public:
    GDALDatasetLoader(std::filesystem::path heightFilePath);

    std::expected<GDALDataset *, GDALLoadError> operator()() const;

  private:
    std::filesystem::path m_heightFilePath;
};

/// Read the elevation at a geographic (lat/lon, degrees) location from the
/// height raster at `path`. Throws on open, reprojection, or read failures.
double GetHeightAtLocationFromGDAL(const std::string &path, double latDeg, double lonDeg);
} // namespace star::terrain
