#pragma once

#include <optional>
#include <string>

#include <glm/vec3.hpp>

struct GLFWwindow;
enum class AntiAliasingMode;
class Atmosphere;
class Camera;
class Player;
class World;

class DebugOverlay
{
public:
    explicit DebugOverlay(GLFWwindow* window);
    ~DebugOverlay();

    DebugOverlay(const DebugOverlay&) = delete;
    DebugOverlay& operator=(const DebugOverlay&) = delete;

    void beginFrame();
    [[nodiscard]] std::optional<int> draw(
        World& world,
        Player& player,
        Camera& camera,
        Atmosphere& atmosphere,
        AntiAliasingMode& antiAliasingMode,
        const glm::vec3& playerPosition,
        bool& fastLeaves
    );
    void setWorldResetStatus(std::string status);
    void render() const;

private:
    bool initialized = false;
    bool newWorldSeedInitialized_ = false;
    int newWorldSeed_ = 1337;
    std::string worldResetStatus_;
};
