#pragma once

#include <star_terrain/file_data/coverage_info/CoverageInfo.hpp>

#include <starlight/core/CommandBus.hpp>

#include <string>
#include <future>

namespace star::terrain
{
class TerrainShapeInfoLoader
{
  public:
    static std::future<CoverageInfo> SubmitForRead(std::filesystem::path filePath,
                                                    const star::core::CommandBus &cmdBus);

    int operator()(const std::filesystem::path &filePath);

    std::future<CoverageInfo> getFuture()
    {
        return m_shapeInfo.get_future();
    }

  private:
    std::promise<CoverageInfo> m_shapeInfo;

    CoverageInfo load(const std::string &filePath) const;
};
} // namespace star::terrain