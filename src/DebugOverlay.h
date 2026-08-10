#pragma once

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
    void draw(
        World& world,
        Player& player,
        Camera& camera,
        Atmosphere& atmosphere,
        AntiAliasingMode& antiAliasingMode,
        const glm::vec3& playerPosition,
        bool& fastLeaves
    );
    void render() const;

private:
    bool initialized = false;
};
