# Starlight-Terrain

Terrain library for the Starlight engine. Provides GeoTIFF-based terrain mesh
generation, chunk layout, and a renderable `StarObject` subclass for the engine's
command pipeline.

## Top-level include groups (`include/star_terrain/`)

| Group            | Purpose                                                                 |
|------------------|-------------------------------------------------------------------------|
| `file_data/`     | Pure data structs describing terrain file inputs (no JSON glue here)   |
| `serialization/` | `nlohmann::json` `to_json`/`from_json` for the `file_data/` structs      |
| `generated/`     | `TerrainChunk` — loads a GeoTIFF region and produces a `StarMesh`        |
| `processor/`     | TBB-based chunk processing helpers (`TerrainChunkProcessor`)            |
| `layout/`        | `TerrainGrid` — arranges chunks into a 2D grid by center coordinates   |
| `io/`            | Async terrain-file readers (`TerrainShapeInfoLoader`, `ReadResult`)     |
| `rendering/`     | `TerrainObject` (renderable), `FromTerrainDirLoader` (command loader),  |
|                  | `TerrainObjectDefinition`, `TerrainRenderingType`                       |
| `util/`          | Geo-coordinate conversions (`Distance`)                                |

## Public types

- `star::terrain::TerrainObject` — `star::StarObject` subclass; produces one mesh
  per chunk described by `height_info.json`. Construct with a
  `TerrainObjectDefinition` (terrain dir + shader paths + render type).
- `star::terrain::FromTerrainDirLoader` —
  `star::command::create_object::ObjectLoader` subclass; plug into
  `star::command::CreateObject::Builder::setLoader(...)` to load terrain through
  the engine's commandBus, mirroring `FromObjFileLoader`.
- `star::terrain::rendering::Type` — `enum class` (`Flat`, `Real`) with
  `toString(Type)`.
- `star::terrain::TerrainChunk`, `ChunkInfo`, `CoverageInfo`, `TextureDataInfo`,
  `TerrainGrid`, `CoverageInfo` loaders.

## CMake

Target: `Starlight::terrain` (alias of `starlight_terrain`).

Public dependencies: `Starlight::starlight`, `starlight_common`, `GDAL::GDAL`,
`TBB::tbb`, `nlohmann_json`.

## Known TODOs

- `io/ReadResult` currently only defines `Result::success`. Needs `failure`,
  `fileNotFound`, `parseError` and a message field before it can meaningfully
  signal errors from `ReadTerrainTextureInfo`.