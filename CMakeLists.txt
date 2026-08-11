cmake_minimum_required(VERSION 3.26)

project(starlight_terrain
    DESCRIPTION "Terrain library for Starlight"
    LANGUAGES CXX
    VERSION 1.0.14
)

option(STARLIGHT_TERRAIN_BUILD_SHARED "Build as shared library" OFF)

set(LIBTYPE STATIC)
if(STARLIGHT_TERRAIN_BUILD_SHARED)
    set(LIBTYPE SHARED)
endif()

find_package(GDAL CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(TBB CONFIG REQUIRED)
find_package(hwy CONFIG REQUIRED)
find_package(starlight_common CONFIG REQUIRED)

if(NOT TARGET starlight_common)
    find_package(starlight_common CONFIG REQUIRED)
endif()

if(NOT TARGET Starlight::starlight)
    find_package(starlight CONFIG REQUIRED)
endif()

set(${PROJECT_NAME}_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/util/Distance.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/generated/terrain_chunk/TerrainChunk.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/generated/terrain_chunk/TerrainTransform.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainRenderingType.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainObjectDefinition.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainObject.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/FromTerrainDirLoader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainVertexDescription.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/layout/TerrainGrid.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/io/ReadResult.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/io/TerrainShapeInfoLoader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/TextureDataInfo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/texture_data/Reader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/ChunkInfo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/CoverageInfo.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/TextureDataInfo_json.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/ChunkInfo_json.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/file_data/CoverageInfo_json.hpp
)

set(${PROJECT_NAME}_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/util/Distance.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/generated/terrain_chunk/TerrainChunk.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/generated/terrain_chunk/TerrainTransform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/TerrainObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/FromTerrainDirLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/TerrainVertexDescription.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/layout/TerrainGrid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/io/TerrainShapeInfoLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/file_data/texture_data/Reader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/file_data/TextureDataInfo_json.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/file_data/ChunkInfo_json.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/file_data/CoverageInfo_json.cpp
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
        TBB::tbb
        hwy::hwy
    PRIVATE
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

# Bundle PROJ runtime data so coordinate transforms work from a
# bare tar extract with no install step and no PROJ_LIB env var.
# libproj.so is installed to ${CMAKE_INSTALL_LIBDIR} by the consuming app
# (via RUNTIME_DEPENDENCIES); PROJ resolves its data dir relative to the
# library as <libdir>/../share/proj, so installing proj.db to
# ${CMAKE_INSTALL_DATADIR}/proj (share/proj) makes it auto-discovered.
set(_STARLIGHT_TERRAIN_PROJ_CANDIDATES
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/proj"
    "${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/proj"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/proj"
    "${CMAKE_SOURCE_DIR}/extern/vcpkg/installed/${VCPKG_TARGET_TRIPLET}/share/proj"
)
set(_STARLIGHT_TERRAIN_PROJ_DATA_DIR "")
foreach(_c ${_STARLIGHT_TERRAIN_PROJ_CANDIDATES})
    if(EXISTS "${_c}/proj.db")
        set(_STARLIGHT_TERRAIN_PROJ_DATA_DIR "${_c}")
        break()
    endif()
endforeach()
if(_STARLIGHT_TERRAIN_PROJ_DATA_DIR)
    install(DIRECTORY "${_STARLIGHT_TERRAIN_PROJ_DATA_DIR}/"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/proj"
            FILES_MATCHING
                PATTERN "proj.db"
                PATTERN "proj.ini"
                PATTERN "*.json"
                PATTERN "*.tif"
                PATTERN "*.lst"
                PATTERN "vcpkg*.json" EXCLUDE)
    message(STATUS "Bundling PROJ data: ${_STARLIGHT_TERRAIN_PROJ_DATA_DIR} -> ${CMAKE_INSTALL_DATADIR}/proj")
else()
    message(WARNING "PROJ data (proj.db) not found under the vcpkg installed tree; "
                    "runtime coordinate transforms (e.g. EPSG:4326<->3857) will fail. "
                    "Ship proj.db and set PROJ_LIB on the target, or fix VCPKG_INSTALLED_DIR.")
endif()
