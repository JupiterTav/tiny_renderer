#include "Shader.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <utility>

Shader::Shader(const std::filesystem::path &vertexPath,
               const std::filesystem::path &fragmentPath) {
  load(vertexPath, fragmentPath);
}

Shader::~Shader() { destroy(); }

Shader::Shader(Shader &&other) noexcept
    : program_(std::exchange(other.program_, 0)),
      uniformCache_(std::move(other.uniformCache_)) {
  other.uniformCache_.clear();
}

Shader &Shader::operator=(Shader &&other) noexcept {
  if (this != &other) {
    destroy();
    program_ = std::exchange(other.program_, 0);
    uniformCache_ = std::move(other.uniformCache_);
    other.uniformCache_.clear();
  }
  return *this;
}

bool Shader::load(const std::filesystem::path &vertexPath,
                  const std::filesystem::path &fragmentPath) {
  std::string vertexSource = readFile(vertexPath);
  if (vertexSource.empty()) {
    std::fprintf(stderr, "Shader::Error::empty vertex shader '%s'\n",
                 vertexPath.string().c_str());
    return false;
  }

  std::string fragmentSource = readFile(fragmentPath);
  if (fragmentSource.empty()) {
    std::fprintf(stderr, "Shader::Error::empty fragment shader '%s'\n",
                 fragmentPath.string().c_str());
    return false;
  }

  destroy();

  GLuint vertexShader = compile(GL_VERTEX_SHADER, vertexSource, vertexPath);
  if (vertexShader == 0)
    return false;

  GLuint fragmentShader =
      compile(GL_FRAGMENT_SHADER, fragmentSource, fragmentPath);
  if (fragmentShader == 0) {
    glDeleteShader(vertexShader);
    return false;
  }

  return link(vertexShader, fragmentShader);
}

bool Shader::loadFromSource(const std::string &vertexSource,
                            const std::string &fragmentSource) {
  if (vertexSource.empty() || fragmentSource.empty()) {
    std::fprintf(stderr, "Shader::Error::empty shader source\n");
    return false;
  }

  destroy();

  GLuint vertexShader =
      compile(GL_VERTEX_SHADER, vertexSource, "<vertex source>");
  if (vertexShader == 0)
    return false;

  GLuint fragmentShader =
      compile(GL_FRAGMENT_SHADER, fragmentSource, "<fragment source>");
  if (fragmentShader == 0) {
    glDeleteShader(vertexShader);
    return false;
  }

  return link(vertexShader, fragmentShader);
}

void Shader::destroy() {
  if (program_ != 0) {
    glDeleteProgram(program_);
    program_ = 0;
  }
  uniformCache_.clear();
}

void Shader::use() const {
  if (program_ != 0)
    glUseProgram(program_);
}

void Shader::unuse() const { glUseProgram(0); }

std::string Shader::readFile(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (!file) {
    std::fprintf(stderr, "Shader::Error::could not open '%s'\n",
                 path.string().c_str());
    return {};
  }

  std::ostringstream contents;
  contents << file.rdbuf();
  return contents.str();
}

GLuint Shader::compile(GLenum type, const std::string &source,
                       const std::filesystem::path &path) {
  GLuint shader = glCreateShader(type);
  const char *sourceCString = source.c_str();
  glShaderSource(shader, 1, &sourceCString, nullptr);
  glCompileShader(shader);

  GLint success = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(length > 0 ? static_cast<std::size_t>(length) : 1, '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    const char *stage = type == GL_VERTEX_SHADER ? "vertex" : "fragment";
    std::fprintf(stderr, "Shader::Error::%s compile failed (%s):\n%s\n", stage,
                 path.string().c_str(), log.c_str());
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

bool Shader::link(GLuint vertexShader, GLuint fragmentShader) {
  program_ = glCreateProgram();
  glAttachShader(program_, vertexShader);
  glAttachShader(program_, fragmentShader);
  glLinkProgram(program_);

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  GLint success = GL_FALSE;
  glGetProgramiv(program_, GL_LINK_STATUS, &success);
  if (!success) {
    GLint length = 0;
    glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &length);
    std::string log(length > 0 ? static_cast<std::size_t>(length) : 1, '\0');
    glGetProgramInfoLog(program_, length, nullptr, log.data());
    std::fprintf(stderr, "Shader::Error::link failed:\n%s\n", log.c_str());
    destroy();
    return false;
  }

  return true;
}

GLint Shader::uniformLocation(const std::string &name) const {
  auto it = uniformCache_.find(name);
  if (it != uniformCache_.end())
    return it->second;

  GLint location = glGetUniformLocation(program_, name.c_str());
  uniformCache_.emplace(name, location);
  return location;
}

void Shader::setBool(const std::string &name, bool value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniform1i(uniformLocation(name), value ? 1 : 0);
}

void Shader::setInt(const std::string &name, int value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniform1i(uniformLocation(name), value);
}

void Shader::setUint(const std::string &name, unsigned int value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniform1ui(uniformLocation(name), value);
}

void Shader::setFloat(const std::string &name, float value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniform1f(uniformLocation(name), value);
}

void Shader::setVec2(const std::string &name, const glm::vec2 &value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniform2fv(uniformLocation(name), 1, &value[0]);
}

void Shader::setVec3(const std::string &name, const glm::vec3 &value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniform3fv(uniformLocation(name), 1, &value[0]);
}

void Shader::setVec4(const std::string &name, const glm::vec4 &value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniform4fv(uniformLocation(name), 1, &value[0]);
}

void Shader::setIVec2(const std::string &name, const glm::ivec2 &value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniform2iv(uniformLocation(name), 1, &value[0]);
}

void Shader::setIVec3(const std::string &name, const glm::ivec3 &value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniform3iv(uniformLocation(name), 1, &value[0]);
}

void Shader::setIVec4(const std::string &name, const glm::ivec4 &value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniform4iv(uniformLocation(name), 1, &value[0]);
}

void Shader::setMat3(const std::string &name, const glm::mat3 &value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniformMatrix3fv(uniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

void Shader::setMat4(const std::string &name, const glm::mat4 &value) const {
  if (program_ == 0)
    return;
  glUseProgram(program_);
  glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, &value[0][0]);
}

void Shader::setFloatArray(const std::string &name, const float *values,
                           GLsizei count) const {
  if (program_ == 0 || values == nullptr)
    return;
  glUseProgram(program_);
  glUniform1fv(uniformLocation(name), count, values);
}
