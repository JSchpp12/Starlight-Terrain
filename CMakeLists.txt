cmake_minimum_required(VERSION 3.26)

project(starlight_terrain
    DESCRIPTION "Terrain library for Starlight"
    LANGUAGES CXX
    VERSION 1.0.0
)

option(STARLIGHT_TERRAIN_BUILD_SHARED "Build as shared library" OFF)

set(LIBTYPE STATIC)
if(STARLIGHT_TERRAIN_BUILD_SHARED)
    set(LIBTYPE SHARED)
endif()

find_package(GDAL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(starlight_common CONFIG REQUIRED)

if(NOT TARGET starlight_common)
    find_package(starlight_common CONFIG REQUIRED)
endif()

if(NOT TARGET Starlight::starlight)
    find_package(starlight CONFIG REQUIRED)
endif()

set(${PROJECT_NAME}_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainRenderingType.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/io/TerrainShapeInfoLoader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/texture_data_info/TextureDataInfo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/texture_data_info/TextureDataInfo_json.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/texture_data_info/Reader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/chunk_info/ChunkInfo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/chunk_info/ChunkInfo_json.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/coverage_info/CoverageInfo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/coverage_info/CoverageInfo_json.hpp
)

set(${PROJECT_NAME}_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/io/TerrainShapeInfoLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/file_data/texture_data_info/Reader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/file_data/texture_data_info/TextureDataInfo_json.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/file_data/chunk_info/ChunkInfo_json.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/file_data/coverage_info/CoverageInfo_json.cpp
)

add_library(${PROJECT_NAME} ${LIBTYPE}
    ${${PROJECT_NAME}_HEADERS}
    ${${PROJECT_NAME}_SOURCES}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Starlight::starlight
        starlight_common
        GDAL::GDAL
    PRIVATE
        # Add private deps here
)

add_library(Starlight::terrain ALIAS ${PROJECT_NAME})

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS ${PROJECT_NAME}
    EXPORT starlight_terrainTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT starlight_terrainTargets
    FILE        starlight_terrainTargets.cmake
    NAMESPACE   Starlight::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/starlight_terrain
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/starlight_terrainConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/starlight_terrainConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/starlight_terrain
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/starlight_terrainConfigVersion.cmake
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/starlight_terrainConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/starlight_terrainConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/starlight_terrain
)