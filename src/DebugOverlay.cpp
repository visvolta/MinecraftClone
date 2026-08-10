#include "DebugOverlay.h"

#include "Atmosphere.h"
#include "Camera.h"
#include "Chunk.h"
#include "World.h"
#include "Player.h"
#include "PostProcessor.h"
#include "worldgen/Biome.h"

#include <cmath>
#include <stdexcept>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace
{
int worldToChunk(float coordinate, int chunkSize)
{
    return static_cast<int>(
        std::floor(coordinate / static_cast<float>(chunkSize))
    );
}
}

DebugOverlay::DebugOverlay(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // The GLFW backend supplies DisplayFramebufferScale. InventoryUI uses it
    // to map each source GUI pixel to exactly three physical framebuffer
    // pixels on Windows DPI-scaled monitors.
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
    {
        throw std::runtime_error(
            "Failed to initialize Dear ImGui GLFW backend"
        );
    }

    if (!ImGui_ImplOpenGL3_Init("#version 330 core"))
    {
        throw std::runtime_error(
            "Failed to initialize Dear ImGui OpenGL backend"
        );
    }

    initialized = true;
}

DebugOverlay::~DebugOverlay()
{
    if (!initialized)
    {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DebugOverlay::beginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DebugOverlay::draw(
    World& world,
    Player& player,
    Camera& camera,
    Atmosphere& atmosphere,
    AntiAliasingMode& antiAliasingMode,
    const glm::vec3& playerPosition,
    bool& fastLeaves)
{
    const ImGuiIO& io = ImGui::GetIO();

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav;

    const int chunkX = worldToChunk(playerPosition.x, Chunk::WIDTH);
    const int chunkZ = worldToChunk(playerPosition.z, Chunk::DEPTH);
    const BiomeId biome = world.getBiomeAt(
        static_cast<int>(std::floor(playerPosition.x)),
        static_cast<int>(std::floor(playerPosition.z))
    );

    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.76f);
    ImGui::Begin("Debug", nullptr, flags);

    ImGui::Text("FPS: %.0f", io.Framerate);
    ImGui::Text(
        "Frame: %.2f ms",
        io.Framerate > 0.0f ? 1000.0f / io.Framerate : 0.0f
    );

    ImGui::Separator();
    ImGui::Text(
        "XYZ: %.2f / %.2f / %.2f",
        playerPosition.x,
        playerPosition.y,
        playerPosition.z
    );
    ImGui::Text("Chunk XZ: %d / %d", chunkX, chunkZ);
    ImGui::Text("Biome: %s", biomeName(biome));
    ImGui::Text("Day time: %d", atmosphere.getDayTime());

    ImGui::Separator();
    ImGui::Text("Chunks: %zu", world.getLoadedChunkCount());
    ImGui::Text(
        "Drawn / culled: %d / %d",
        world.getDrawnChunkCount(),
        world.getCulledChunkCount()
    );
    ImGui::Text(
        "Generation queue: %zu",
        world.getPendingChunkCount()
    );
    ImGui::Text("Mesh queue: %zu", world.getPendingMeshCount());
    ImGui::Text("Lighting queue: %zu", world.getPendingLightingCount());
    ImGui::Text("Fluid ticks: %zu", world.getPendingFluidTickCount());
    ImGui::Text("Faces: %d", world.getVisibleFaceCount());
    ImGui::Text("Indexed vertices: %d", world.getVertexCount());

    ImGui::Separator();
    ImGui::Text(
        "Terrain worker: %.2f ms",
        world.getTerrainMilliseconds()
    );
    ImGui::Text(
        "Mesh worker: %.2f ms",
        world.getMeshMilliseconds()
    );
    ImGui::Text(
        "GPU upload: %.2f ms",
        world.getUploadMilliseconds()
    );
    ImGui::Text(
        "Lighting: %.2f ms",
        world.getLightingMilliseconds()
    );
    ImGui::Text(
        "World update: %.2f ms",
        world.getWorldUpdateMilliseconds()
    );


    ImGui::SeparatorText("Controls");

    int renderDistance = world.getRenderDistance();
    if (ImGui::SliderInt(
            "Render distance",
            &renderDistance,
            2,
            16,
            "%d chunks"))
    {
        world.setRenderDistance(renderDistance);
    }

    float fieldOfView = camera.getZoom();
    if (ImGui::SliderFloat(
            "Field of view",
            &fieldOfView,
            30.0f,
            110.0f,
            "%.0f degrees"))
    {
        camera.setZoom(fieldOfView);
    }

    ImGui::Checkbox("Fast leaves (opaque)", &fastLeaves);

    bool smoothLighting = world.isSmoothLightingEnabled();
    if (ImGui::Checkbox("Smooth lighting", &smoothLighting))
        world.setSmoothLightingEnabled(smoothLighting);

    constexpr AntiAliasingMode antiAliasingModes[] = {
        AntiAliasingMode::Off,
        AntiAliasingMode::Fxaa,
        AntiAliasingMode::Smaa,
        AntiAliasingMode::Taa
    };
    const char* antiAliasingPreview =
        antiAliasingModeName(antiAliasingMode);
    if (ImGui::BeginCombo("Anti-aliasing", antiAliasingPreview))
    {
        for (AntiAliasingMode mode : antiAliasingModes)
        {
            const bool selected = mode == antiAliasingMode;
            if (ImGui::Selectable(
                    antiAliasingModeName(mode),
                    selected))
            {
                antiAliasingMode = mode;
            }
            if (selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (antiAliasingMode == AntiAliasingMode::Taa)
        ImGui::TextDisabled("Temporal mode reduces distant leaf shimmer");

    int dayTime = atmosphere.getDayTime();
    if (ImGui::SliderInt("Day time", &dayTime, 0, 23999))
        atmosphere.setDayTime(dayTime);

    bool noClip = player.isNoClip();
    if (ImGui::Checkbox("Noclip fly", &noClip))
        player.setNoClip(noClip);

    if (player.isNoClip())
        ImGui::TextDisabled("Fly: WASD, Space, Shift");

    ImGui::End();
}

void DebugOverlay::render() const
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
