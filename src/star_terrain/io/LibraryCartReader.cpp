#include "star_terrain/io/LibraryCartReader.hpp"

#include "star_terrain/file_data/CoverageInfo.hpp"
#include "star_terrain/file_data/CoverageInfo_json.hpp"
#include "star_terrain/file_data/texture_data/Reader.hpp"
#include "star_terrain/io/GDALDatasetLoader.hpp"
#include "star_terrain/io/IFilter.hpp"
#include "star_terrain/io/LibraryCart.hpp"

#include <starlight/core/logging/LoggingFactory.hpp>

#include <exception>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

namespace star::terrain
{
LibraryCartReader::LibraryCartReader(std::string terrainShapeFile, std::vector<std::unique_ptr<IFilter>> filters)
    : m_payload(std::make_unique<Payload>(
          Payload{.terrainShapeFile = std::move(terrainShapeFile), .filters = std::move(filters)}))
{
}

LibraryCartReader::Result LibraryCartReader::read(const std::filesystem::path &terrainRoot) const
{
    if (m_payload == nullptr || m_payload->terrainShapeFile.empty())
        throw std::runtime_error("Library cart reader has no terrain shape file");

    const std::string &terrainShapeFile = m_payload->terrainShapeFile;

    star::core::logging::debug("Building terrain library cart for '", terrainRoot.string(),
                               "' using shape descriptor '", terrainShapeFile, "'");

    auto [readResult, fileInfo] = ReadTerrainTextureInfo((terrainRoot / "height_info.json").string());
    (void)readResult;

    const auto chunkCount = fileInfo.chunks.size();
    star::core::logging::debug("Terrain '", terrainRoot.string(), "' declares ", chunkCount, " target chunk(s)");

    const auto shapePath = terrainRoot / std::filesystem::path(terrainShapeFile);
    std::ifstream shapeStream(shapePath);
    if (!shapeStream.is_open())
        throw std::runtime_error("Failed to open terrain shape file: " + shapePath.string());

    const CoverageInfo shapeInfo = nlohmann::json::parse(shapeStream).get<CoverageInfo>();
    star::core::logging::debug("Terrain shape center (lat, lon) = (", shapeInfo.center.x, ", ", shapeInfo.center.y,
                               "), view distance = ", shapeInfo.viewDistance);

    const auto fullHeightFilePath = terrainRoot / std::filesystem::path(fileInfo.fullHeightFilePath);
    if (!std::filesystem::exists(fullHeightFilePath))
    {
        std::ostringstream oss;
        oss << "Elevation file does not exist: " << fullHeightFilePath.string()
            << ". The terrain directory is expected to contain the height raster named in "
            << "height_info.json (fullHeightFilePath = '" << fileInfo.fullHeightFilePath << "').";
        throw std::runtime_error(oss.str());
    }

    glm::dvec3 worldCenter{shapeInfo.center.x, shapeInfo.center.y, 0.0};
    if (!fileInfo.chunks.empty())
    {
        worldCenter.z =
            GetHeightAtLocationFromGDAL(fullHeightFilePath.string(), shapeInfo.center.x, shapeInfo.center.y);
    }
    star::core::logging::debug("Terrain world offset = (", worldCenter.x, ", ", worldCenter.y, ", ", worldCenter.z,
                               ")");

    auto cart = std::make_unique<LibraryCart>(
        TerrainTargetInfo{.fullHeightFile = fullHeightFilePath.string(), .offset = worldCenter},
        std::move(fileInfo.chunks));

    for (const auto &filter : m_payload->filters)
    {
        cart->applyFilter(filter.get());
    }

    star::core::logging::info("Built terrain library cart for '", terrainRoot.string(), "': ", cart->chunks().size(),
                              " chunk(s) kept from ", chunkCount, " declared, height raster '",
                              fullHeightFilePath.string(), "'");
    return cart;
}

int LibraryCartReader::operator()(const std::filesystem::path &terrainRoot)
{
    Result result = [&]() -> Result {
        try
        {
            return read(terrainRoot);
        }
        catch (const std::exception &e)
        {
            star::core::logging::warning("Failed to read terrain library cart for '", terrainRoot.string(),
                                         "': ", e.what());
            return std::unexpected(CartReadException{e.what()});
        }
        catch (...)
        {
            star::core::logging::warning("Failed to read terrain library cart for '", terrainRoot.string(),
                                         "': unknown error");
            return std::unexpected(CartReadException{"Terrain library cart read failed"});
        }
    }();

    m_result.set_value(std::move(result));
    return 0;
}
} // namespace star::terrain
