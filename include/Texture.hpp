#pragma once

#include <glad/gl.h>

#include <filesystem>

class Texture {
public:
  Texture() = default;
  explicit Texture(const std::filesystem::path &path);
  ~Texture();

  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;
  Texture(Texture &&other) noexcept;
  Texture &operator=(Texture &&other) noexcept;

  // Color images should use sRGB (the default); data maps such as metallic
  // or roughness must be loaded as linear values.
  bool load(const std::filesystem::path &path, bool srgb = true);
  void destroy();

  void bind(unsigned int unit = 0) const;
  void unbind() const;

  GLuint getTexture() const { return textureID; }
  int width() const { return width_; }
  int height() const { return height_; }
  bool valid() const { return textureID != 0; }

private:
  GLuint textureID{0};
  int width_{0};
  int height_{0};
  int nrChannels_{0};
};
