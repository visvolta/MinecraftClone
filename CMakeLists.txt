
cmake_minimum_required(VERSION 3.20)

project(MinecraftClone VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# This is a real-time game and the normal Windows workflow launches
# build/Debug/MinecraftClone.exe. Fully unoptimized voxel lighting/meshing can
# turn a bounded chunk job into a 100+ ms frame, so retain debug symbols while
# optimizing game code in Debug as well. This can be disabled when stepping
# through generated assembly is specifically required.
option(MINECRAFT_CLONE_OPTIMIZE_DEBUG
       "Enable optimized game code in Debug builds" ON)
if(MINECRAFT_CLONE_OPTIMIZE_DEBUG)
    if(MSVC)
        string(REPLACE "/Od" "/O2" CMAKE_C_FLAGS_DEBUG
               "${CMAKE_C_FLAGS_DEBUG}")
        string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG
               "${CMAKE_C_FLAGS_DEBUG}")
        string(REPLACE "/Od" "/O2" CMAKE_CXX_FLAGS_DEBUG
               "${CMAKE_CXX_FLAGS_DEBUG}")
        string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG
               "${CMAKE_CXX_FLAGS_DEBUG}")
    else()
        add_compile_options($<$<CONFIG:Debug>:-O2>)
    endif()
endif()

include(FetchContent)
find_package(Threads REQUIRED)

# GLAD 2 uses Python/Jinja2 to generate its loader. Fail early with a clear
# message instead of producing an obscure MSBuild custom-command error.
find_package(Python3 COMPONENTS Interpreter REQUIRED)
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import jinja2"
    RESULT_VARIABLE MINECRAFT_CLONE_JINJA2_RESULT
    OUTPUT_QUIET
    ERROR_QUIET
)
if(NOT MINECRAFT_CLONE_JINJA2_RESULT EQUAL 0)
    message(FATAL_ERROR
        "Python module 'jinja2' is required by GLAD 2. Install it with: "
        "${Python3_EXECUTABLE} -m pip install jinja2"
    )
endif()

# Keep dependency builds small.
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLM_BUILD_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)

# GLAD 2 generates the OpenGL loader during CMake configuration.

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.8
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR cmake
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(glfw glm glad imgui nlohmann_json)

glad_add_library(
    glad_gl_core_33
    REPRODUCIBLE
    API gl:core=3.3
)

# Release 1.12 migration modules. These targets establish a stable dependency
# direction for future content packs and compiled mods while the legacy game
# systems are moved out of the executable incrementally.
add_library(minecraft_core STATIC
    src/core/ResourceLocation.cpp
)
target_include_directories(minecraft_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_library(minecraft_engine STATIC
    src/engine/FixedStepClock.cpp
)
target_include_directories(minecraft_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(minecraft_engine PUBLIC minecraft_core)

add_library(minecraft_content STATIC
    src/BlockProperties.cpp
    src/BlockShape.cpp
    src/Item.cpp
    src/content/ContentCatalog.cpp
    src/content/BlockStateLogic.cpp
    src/content/MinecraftContent.cpp
    src/content/resources/ResourcePack.cpp
)
target_include_directories(minecraft_content PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(minecraft_content PUBLIC
    minecraft_core
    glm::glm
    nlohmann_json::nlohmann_json
)

add_library(minecraft_game STATIC
    src/game/GameBootstrap.cpp
    src/gameplay/GameplayRegistries.cpp
    src/gameplay/SurvivalStats.cpp
)
target_include_directories(minecraft_game PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(minecraft_game PUBLIC
    minecraft_content
    minecraft_engine
)

add_library(minecraft_client STATIC
    src/Mesh.cpp
    src/Shader.cpp
    src/Texture2D.cpp
    src/TextureAtlas.cpp
    src/client/render/ModelBakery.cpp
    src/client/render/MobModel.cpp
    src/client/render/RuntimeTextureAtlas.cpp
)
target_include_directories(minecraft_client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
)
target_link_libraries(minecraft_client PUBLIC
    minecraft_core
    minecraft_content
    glad_gl_core_33
    glm::glm
)

add_library(minecraft_world STATIC
    src/Chunk.cpp
    src/ChunkManager.cpp
    src/TerrainGenerator.cpp
    src/gameplay/FarmingSystem.cpp
    src/gameplay/RedstoneSystem.cpp
    src/worldgen/BetaNoiseGeneratorOctaves.cpp
    src/worldgen/BetaNoiseGeneratorPerlin.cpp
    src/worldgen/BetaSimplexNoise.cpp
    src/worldgen/BetaSimplexOctaves.cpp
    src/worldgen/CaveGenerator.cpp
    src/worldgen/ClayGenerator.cpp
    src/worldgen/DecorationGenerator.cpp
    src/worldgen/DungeonGenerator.cpp
    src/worldgen/LakeGenerator.cpp
    src/worldgen/MinableGenerator.cpp
    src/worldgen/OreGenerator.cpp
    src/worldgen/PopulationGenerator.cpp
    src/worldgen/RavineGenerator.cpp
    src/worldgen/StructureGenerator.cpp
    src/worldgen/WorldGenerationContext.cpp
    src/worldgen/Biome.cpp
    src/worldgen/BiomeMap.cpp
    src/worldgen/JavaRandom.cpp
    src/worldgen/SurfaceBuilder.cpp
    src/worldgen/TreeGenerator.cpp
    src/worldgen/VanillaBiomeLayer.cpp
)
target_include_directories(minecraft_world PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(minecraft_world PUBLIC
    minecraft_content
    glm::glm
)


add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_compile_definitions(imgui PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(imgui PUBLIC glfw glad_gl_core_33)

set(MINECRAFT_CLONE_SOURCES
    src/main.cpp
    src/client/ClientApplication.cpp
    src/AssetPaths.cpp
    src/AnimatedTexture.cpp
    src/Atmosphere.cpp
    src/BlockOutline.cpp
    src/BlockDamageOverlay.cpp
    src/BlockEntity.cpp
    src/BlockBreakingController.cpp
    src/BlockDrops.cpp
    src/BlockMeshBuilder.cpp
    src/BiomeColorMap.cpp
    src/Camera.cpp
    src/CameraEffects.cpp
    src/ChunkMesh.cpp
    src/ChunkMeshing.cpp
    src/CraftingRecipe.cpp
    src/DeathScreen.cpp
    src/DebugOverlay.cpp
    src/Frustum.cpp
    src/Furnace.cpp
    src/FluidSystem.cpp
    src/FluidMeshBuilder.cpp
    src/FluidTextures.cpp
    src/LightingSystem.cpp
    src/Inventory.cpp
    src/InventoryUI.cpp
    src/ItemAtlas.cpp
    src/ItemEntity.cpp
    src/ItemEntityManager.cpp
    src/ItemEntityRenderer.cpp
    src/client/render/MobEntityRenderer.cpp
    src/entity/EntityUuid.cpp
    src/entity/MobEntity.cpp
    src/entity/MobEntityManager.cpp
    src/entity/ai/Goal.cpp
    src/entity/ai/MobAiController.cpp
    src/entity/navigation/PathNavigation.cpp
    src/entity/navigation/WorldNavigationBlockAccess.cpp
    src/Player.cpp
    src/PlayerHUD.cpp
    src/PostProcessor.cpp
    src/Raycast.cpp
    src/SaveGame.cpp
    src/SkyRenderer.cpp
    src/ToolProperties.cpp
    src/World.cpp
)

add_executable(MinecraftClone ${MINECRAFT_CLONE_SOURCES})

# GLFW must not include the platform OpenGL header (GL/gl.h). GLAD owns all
# OpenGL declarations and must be the only OpenGL loader in the project.
target_compile_definitions(MinecraftClone PRIVATE
    GLFW_INCLUDE_NONE
    MINECRAFT_CLONE_SOURCE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

target_include_directories(MinecraftClone PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external/stb
)

target_link_libraries(MinecraftClone PRIVATE
    glad_gl_core_33
    glfw
    glm::glm
    imgui
    Threads::Threads
    minecraft_game
    minecraft_client
    minecraft_world
)

if(MSVC)
    target_compile_options(MinecraftClone PRIVATE
        /W4
        /permissive-
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:RelWithDebInfo>:/O2>
    )
else()
    target_compile_options(MinecraftClone PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:RelWithDebInfo>:-O3>
    )
endif()

add_custom_command(TARGET MinecraftClone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_FILE_DIR:MinecraftClone>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:MinecraftClone>/assets"
    COMMENT "Replacing copied game assets"
)

if(BUILD_TESTING)
    add_executable(minecraft_registry_tests
        tests/RegistryTests.cpp
    )
    target_include_directories(minecraft_registry_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(minecraft_registry_tests PRIVATE
        minecraft_game
    )
    add_test(NAME minecraft_registry_tests COMMAND minecraft_registry_tests)

    add_executable(minecraft_block_state_tests
        tests/BlockStateTests.cpp
    )
    target_include_directories(minecraft_block_state_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(minecraft_block_state_tests PRIVATE
        minecraft_world
    )
    add_test(NAME minecraft_block_state_tests COMMAND minecraft_block_state_tests)

    add_executable(minecraft_resource_pack_tests
        tests/ResourcePackTests.cpp
    )
    target_include_directories(minecraft_resource_pack_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(minecraft_resource_pack_tests PRIVATE
        minecraft_content
    )
    add_test(NAME minecraft_resource_pack_tests COMMAND minecraft_resource_pack_tests)

    add_executable(minecraft_save_game_tests
        tests/SaveGameTests.cpp
        src/SaveGame.cpp
        src/BlockEntity.cpp
        src/Furnace.cpp
        src/Inventory.cpp
    )
    target_include_directories(minecraft_save_game_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(minecraft_save_game_tests PRIVATE
        minecraft_game
        minecraft_world
        glm::glm
    )
    add_test(NAME minecraft_save_game_tests COMMAND minecraft_save_game_tests)

    add_executable(minecraft_entity_runtime_tests
        tests/EntityRuntimeTests.cpp
        src/entity/ai/Goal.cpp
        src/entity/navigation/PathNavigation.cpp
    )
    target_include_directories(minecraft_entity_runtime_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(minecraft_entity_runtime_tests PRIVATE
        minecraft_game
        glm::glm
    )
    add_test(
        NAME minecraft_entity_runtime_tests
        COMMAND minecraft_entity_runtime_tests
    )

    add_executable(minecraft_gameplay_data_tests
        tests/GameplayDataTests.cpp
        src/BlockDrops.cpp
        src/BlockEntity.cpp
        src/CraftingRecipe.cpp
        src/Furnace.cpp
    )
    target_include_directories(minecraft_gameplay_data_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(minecraft_gameplay_data_tests PRIVATE
        minecraft_game
        nlohmann_json::nlohmann_json
    )
    add_test(NAME minecraft_gameplay_data_tests COMMAND minecraft_gameplay_data_tests)

    add_executable(minecraft_performance_tests
        tests/PerformanceTests.cpp
        src/LightingSystem.cpp
    )
    target_include_directories(minecraft_performance_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(minecraft_performance_tests PRIVATE
        minecraft_game
        minecraft_world
        glad_gl_core_33
        glm::glm
    )
    add_test(NAME minecraft_performance_tests COMMAND minecraft_performance_tests)

    add_executable(minecraft_migration_stages_tests
        tests/MigrationStagesTests.cpp
    )
    target_include_directories(minecraft_migration_stages_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(minecraft_migration_stages_tests PRIVATE
        minecraft_game
        minecraft_world
        minecraft_client
        glm::glm
    )
    add_test(
        NAME minecraft_migration_stages_tests
        COMMAND minecraft_migration_stages_tests
    )

    add_executable(minecraft_world_generation_tests
        tests/WorldGenerationTests.cpp
    )
    target_include_directories(minecraft_world_generation_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(minecraft_world_generation_tests PRIVATE
        minecraft_world
    )
    add_test(
        NAME minecraft_world_generation_tests
        COMMAND minecraft_world_generation_tests
    )
    set_tests_properties(
        minecraft_registry_tests
        minecraft_resource_pack_tests
        minecraft_gameplay_data_tests
        minecraft_entity_runtime_tests
        minecraft_migration_stages_tests
        PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
