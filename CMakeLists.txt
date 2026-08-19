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
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/generated/terrain_chunk/TerrainDataset.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/generated/terrain_chunk/TerrainTransform.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/policies/ShadowDepthReleasePolicy.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/policies/ShadowDepthReacquirePolicy.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainRenderingType.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainObjectDefinition.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainGeometryDefinition.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainObject.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainShadowRenderPhase.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainShadowRenderPhaseProvider.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/FromTerrainDirLoader.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/TerrainVertexDescription.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/ShadowCameraController.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/star_terrain/rendering/ShadowCameraTransfer.hpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/generated/terrain_chunk/TerrainDataset.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/generated/terrain_chunk/TerrainTransform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/TerrainObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/policies/ShadowDepthReleasePolicy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/policies/ShadowDepthReacquirePolicy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/TerrainGeometryDefinition.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/TerrainShadowRenderPhase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/TerrainShadowRenderPhaseProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/FromTerrainDirLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/TerrainVertexDescription.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/ShadowCameraController.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/star_terrain/rendering/ShadowCameraTransfer.cpp
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

if(COMMAND star_add_pch)
    star_add_pch(${PROJECT_NAME})
endif()

# Shared, generated precompiled header (GDAL).
# Other targets add their own PCH from the same generated source via:
#     if(COMMAND star_add_gdal_pch)
#         star_add_gdal_pch(<target>)
#     endif()
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/StarGdalPch.cmake")
star_add_gdal_pch(${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /bigobj
        /MP
        /EHsc
        /Zc:__cplusplus
    )
endif()

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


# --- Build-tree PROJ data (development runs without an install step) ----------
# The install() rules above only fire during `cmake --install` / CPack. When a
# developer builds and runs an executable straight out of the build tree (e.g.
# the Ninja presets), PROJ must still locate proj.db. vcpkg builds PROJ with
# EMBED_PROJ_DATA_PATH=OFF and PROJ_DATA_ENV_VAR_TRIED_LAST=ON, so PROJ resolves
# its data directory *relative to proj_9.dll* as <parent_of_dll_dir>/share/proj.
# vcpkg deploys proj_9.dll next to every executable that links it, and the depth
# of that deploy directory varies by generator and target:
#   - Visual Studio (multi-config), exe/DLL in ${CMAKE_BINARY_DIR}/$<CONFIG>  -> ${CMAKE_BINARY_DIR}/share/proj
#   - Ninja, main exe:               exe/DLL in ${CMAKE_BINARY_DIR}           -> ${CMAKE_BINARY_DIR}/../share/proj
#   - Ninja, test exes:              exe/DLL in ${CMAKE_BINARY_DIR}/tests    -> ${CMAKE_BINARY_DIR}/share/proj
if(_STARLIGHT_TERRAIN_PROJ_DATA_DIR)
    set(_STARLIGHT_TERRAIN_PROJ_BUILD_DIRS
        "${CMAKE_BINARY_DIR}/share/proj"
        "${CMAKE_BINARY_DIR}/../share/proj"
    )
    add_custom_target(bundle_proj_data ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/share/proj"
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                "${_STARLIGHT_TERRAIN_PROJ_DATA_DIR}" "${CMAKE_BINARY_DIR}/share/proj"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/../share/proj"
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
                "${_STARLIGHT_TERRAIN_PROJ_DATA_DIR}" "${CMAKE_BINARY_DIR}/../share/proj"
        COMMENT "Staging PROJ data for build-tree runs: ${_STARLIGHT_TERRAIN_PROJ_DATA_DIR}"
        VERBATIM
    )
    message(STATUS "Staging PROJ data for build-tree runs -> ${CMAKE_BINARY_DIR}/share/proj ; ${CMAKE_BINARY_DIR}/../share/proj")
else()
    message(WARNING "PROJ data (proj.db) not staged into the build tree; running "
                    "the freshly built executable without an install step will "
                    "produce GDAL 'cannot find proj.db' warnings.")
endif()
