#include "Camera.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 worldUp, float yaw, float pitch)
    : position_(position),
      worldUp_(worldUp),
      yaw_(yaw),
      pitch_(pitch)
{
    updateVectors();
}

void Camera::processMousePosition(double xPosition, double yPosition)
{
    if (firstMouseEvent_)
    {
        lastMouseX_ = xPosition;
        lastMouseY_ = yPosition;
        firstMouseEvent_ = false;
        return;
    }

    double xOffset = xPosition - lastMouseX_;
    double yOffset = lastMouseY_ - yPosition;

    lastMouseX_ = xPosition;
    lastMouseY_ = yPosition;

    // Ignore unusually large jumps caused by focus/cursor-mode changes.
    constexpr double maxMouseJump = 250.0;
    xOffset = std::clamp(xOffset, -maxMouseJump, maxMouseJump);
    yOffset = std::clamp(yOffset, -maxMouseJump, maxMouseJump);

    yaw_ += static_cast<float>(xOffset) * mouseSensitivity_;
    pitch_ += static_cast<float>(yOffset) * mouseSensitivity_;
    pitch_ = std::clamp(pitch_, -89.0f, 89.0f);

    updateVectors();
}

void Camera::resetMouseTracking()
{
    firstMouseEvent_ = true;
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position_, position_ + front_, up_);
}

float Camera::getZoom() const
{
    return zoom_;
}

void Camera::setZoom(float degrees)
{
    zoom_ = std::clamp(degrees, 30.0f, 110.0f);
}

const glm::vec3& Camera::getPosition() const
{
    return position_;
}

const glm::vec3& Camera::getForward() const
{
    return front_;
}

const glm::vec3& Camera::getRight() const
{
    return right_;
}

float Camera::getYaw() const noexcept
{
    return yaw_;
}

float Camera::getPitch() const noexcept
{
    return pitch_;
}

void Camera::setPosition(const glm::vec3& position)
{
    position_ = position;
}

void Camera::updateVectors()
{
    glm::vec3 direction;
    direction.x = std::cos(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));
    direction.y = std::sin(glm::radians(pitch_));
    direction.z = std::sin(glm::radians(yaw_)) * std::cos(glm::radians(pitch_));

    front_ = glm::normalize(direction);
    right_ = glm::normalize(glm::cross(front_, worldUp_));
    up_ = glm::normalize(glm::cross(right_, front_));
}
