#include "star_terrain/io/TerrainShapeInfoLoader.hpp"

#include <star_terrain/file_data/coverage_info/CoverageInfo_json.hpp>

#include <starlight/command/FileIO/ReadFromFile.hpp>
#include <starlight/common/helpers/FileHelpers.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/job/tasks/IOTask.hpp>

#include <nlohmann/json.hpp>

#include <fstream>

namespace star::terrain
{
std::future<CoverageInfo> TerrainShapeInfoLoader::SubmitForRead(std::filesystem::path filePath,
                                                                const star::core::CommandBus &cmdBus)
{
    TerrainShapeInfoLoader shapeLoader{};
    auto future = shapeLoader.getFuture();

    auto readPayload = star::job::tasks::io::ReadPayload<TerrainShapeInfoLoader>{
        .filePath = std::move(filePath), .readFunction = std::move(shapeLoader)};
    auto readTask = star::job::tasks::io::CreateReadTask(std::move(readPayload));
    auto readCmd = star::command::file_io::ReadFromFile{std::move(readTask)};

    cmdBus.submit(readCmd);
    return future;
}

int TerrainShapeInfoLoader::operator()(const std::filesystem::path &filePath)
{
    auto data = load(filePath.string());
    m_shapeInfo.set_value(std::move(data));

    return 0;
}

CoverageInfo TerrainShapeInfoLoader::load(const std::string &filePath) const
{
    if (!star::file_helpers::FileExists(filePath))
    {
        std::string msg = "Shape file does not exist: " + filePath;
        STAR_THROW(msg);
    }

    std::ifstream i(filePath);
    auto jData = nlohmann::json::parse(i);

    return jData.get<CoverageInfo>();
}
} // namespace star::terrain