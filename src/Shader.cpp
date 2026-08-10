#include "Shader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
{
    const std::string vertexSource = readFile(vertexPath);
    const std::string fragmentSource = readFile(fragmentPath);

    const GLuint vertexShader = compile(GL_VERTEX_SHADER, vertexSource, vertexPath.string());
    const GLuint fragmentShader = compile(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath.string());

    id_ = glCreateProgram();
    glAttachShader(id_, vertexShader);
    glAttachShader(id_, fragmentShader);
    glLinkProgram(id_);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(id_, GL_LINK_STATUS, &success);
    if (success == GL_FALSE)
    {
        char log[2048]{};
        glGetProgramInfoLog(id_, sizeof(log), nullptr, log);
        glDeleteProgram(id_);
        id_ = 0;
        throw std::runtime_error(std::string("Shader program link failed:\n") + log);
    }
}

Shader::~Shader()
{
    if (id_ != 0)
        glDeleteProgram(id_);
}

Shader::Shader(Shader&& other) noexcept
    : id_(std::exchange(other.id_, 0))
{
}

Shader& Shader::operator=(Shader&& other) noexcept
{
    if (this != &other)
    {
        if (id_ != 0)
            glDeleteProgram(id_);
        id_ = std::exchange(other.id_, 0);
    }
    return *this;
}

void Shader::use() const
{
    glUseProgram(id_);
}

void Shader::setBool(const char* name, bool value) const
{
    glUniform1i(uniformLocation(name), value ? 1 : 0);
}

void Shader::setInt(const char* name, int value) const
{
    glUniform1i(uniformLocation(name), value);
}

void Shader::setFloat(const char* name, float value) const
{
    glUniform1f(uniformLocation(name), value);
}

void Shader::setVec2(const char* name, const glm::vec2& value) const
{
    glUniform2f(uniformLocation(name), value.x, value.y);
}

void Shader::setVec3(const char* name, const glm::vec3& value) const
{
    glUniform3f(uniformLocation(name), value.x, value.y, value.z);
}

void Shader::setVec4(const char* name, const glm::vec4& value) const
{
    glUniform4f(
        uniformLocation(name),
        value.x,
        value.y,
        value.z,
        value.w
    );
}

void Shader::setMat4(const char* name, const glm::mat4& value) const
{
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

std::string Shader::readFile(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Failed to open shader file: " + path.string());

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint Shader::compile(GLenum type, const std::string& source, const std::string& label)
{
    const GLuint shader = glCreateShader(type);
    const char* sourcePointer = source.c_str();
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        char log[2048]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error("Shader compile failed (" + label + "):\n" + log);
    }

    return shader;
}

GLint Shader::uniformLocation(const char* name) const
{
    const GLint location = glGetUniformLocation(id_, name);
    if (location == -1)
        throw std::runtime_error(std::string("Shader uniform not found: ") + name);
    return location;
}
