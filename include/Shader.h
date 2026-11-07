#pragma once

#include <glad/gl.h>

#include <glm/glm.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>

class Shader {
public:
  Shader() = default;
  Shader(const std::filesystem::path &vertexPath,
         const std::filesystem::path &fragmentPath);
  ~Shader();

  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;
  Shader(Shader &&other) noexcept;
  Shader &operator=(Shader &&other) noexcept;

  // Reads, compiles and links the given shader files.
  // Returns false on failure, leaving the shader without a program.
  bool load(const std::filesystem::path &vertexPath,
            const std::filesystem::path &fragmentPath);
  bool loadFromSource(const std::string &vertexSource,
                      const std::string &fragmentSource);
  void destroy();

  void use() const;
  void unuse() const;

  GLuint id() const { return program_; }
  explicit operator bool() const { return program_ != 0; }

  void setBool(const std::string &name, bool value) const;
  void setInt(const std::string &name, int value) const;
  void setUint(const std::string &name, unsigned int value) const;
  void setFloat(const std::string &name, float value) const;

  void setVec2(const std::string &name, const glm::vec2 &value) const;
  void setVec3(const std::string &name, const glm::vec3 &value) const;
  void setVec4(const std::string &name, const glm::vec4 &value) const;

  void setIVec2(const std::string &name, const glm::ivec2 &value) const;
  void setIVec3(const std::string &name, const glm::ivec3 &value) const;
  void setIVec4(const std::string &name, const glm::ivec4 &value) const;

  void setMat3(const std::string &name, const glm::mat3 &value) const;
  void setMat4(const std::string &name, const glm::mat4 &value) const;

  void setFloatArray(const std::string &name, const float *values,
                     GLsizei count) const;

private:
  static std::string readFile(const std::filesystem::path &path);
  static GLuint compile(GLenum type, const std::string &source,
                        const std::filesystem::path &path);
  bool link(GLuint vertexShader, GLuint fragmentShader);

  GLint uniformLocation(const std::string &name) const;

  GLuint program_{0};
  mutable std::unordered_map<std::string, GLint> uniformCache_;
};
