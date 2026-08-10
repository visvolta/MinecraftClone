#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    explicit Camera(
        glm::vec3 position = {0.0f, 0.0f, 3.0f},
        glm::vec3 worldUp = {0.0f, 1.0f, 0.0f},
        float yaw = -90.0f,
        float pitch = 0.0f
    );

    void processMousePosition(double xPosition, double yPosition);
    void resetMouseTracking();

    [[nodiscard]] glm::mat4 getViewMatrix() const;
    [[nodiscard]] float getZoom() const;
    void setZoom(float degrees);
    [[nodiscard]] const glm::vec3& getPosition() const;
    [[nodiscard]] const glm::vec3& getForward() const;
    [[nodiscard]] const glm::vec3& getRight() const;
    [[nodiscard]] float getYaw() const noexcept;
    [[nodiscard]] float getPitch() const noexcept;
    void setPosition(const glm::vec3& position);

private:
    void updateVectors();

    glm::vec3 position_;
    glm::vec3 front_{0.0f, 0.0f, -1.0f};
    glm::vec3 up_{};
    glm::vec3 right_{};
    glm::vec3 worldUp_;

    float yaw_;
    float pitch_;
    float mouseSensitivity_ = 0.10f;
    float zoom_ = 75.0f;

    double lastMouseX_ = 0.0;
    double lastMouseY_ = 0.0;
    bool firstMouseEvent_ = true;
};
