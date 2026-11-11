#include "Texture.hpp"

#include "stb_image.h"

#include <iostream>
#include <string>
#include <utility>

Texture::Texture(const std::filesystem::path &path) { load(path); }

Texture::~Texture() { destroy(); }

Texture::Texture(Texture &&other) noexcept
    : textureID(std::exchange(other.textureID, 0)), width_(other.width_),
      height_(other.height_), nrChannels_(other.nrChannels_) {
  other.width_ = 0;
  other.height_ = 0;
  other.nrChannels_ = 0;
}

Texture &Texture::operator=(Texture &&other) noexcept {
  if (this != &other) {
    destroy();
    textureID = std::exchange(other.textureID, 0);
    width_ = other.width_;
    height_ = other.height_;
    nrChannels_ = other.nrChannels_;
    other.width_ = 0;
    other.height_ = 0;
    other.nrChannels_ = 0;
  }
  return *this;
}

bool Texture::load(const std::filesystem::path &path) {
  const std::string pathString = path.string();

  // OpenGL texture coordinates start at the bottom-left, while image files
  // start at the top-left.
  stbi_set_flip_vertically_on_load(true);

  unsigned char *data =
      stbi_load(pathString.c_str(), &width_, &height_, &nrChannels_, 0);
  if (data == nullptr) {
    std::cerr << "Texture::Error::failed to load '" << pathString
              << "': " << stbi_failure_reason() << std::endl;
    width_ = height_ = nrChannels_ = 0;
    return false;
  }

  const int width = width_;
  const int height = height_;
  const int channels = nrChannels_;

  GLenum format = GL_RGB;
  GLint internalFormat = GL_RGB8;
  switch (channels) {
  case 1:
    format = GL_RED;
    internalFormat = GL_R8;
    break;
  case 2:
    format = GL_RG;
    internalFormat = GL_RG8;
    break;
  case 3:
    format = GL_RGB;
    internalFormat = GL_RGB8;
    break;
  case 4:
    format = GL_RGBA;
    internalFormat = GL_RGBA8;
    break;
  default:
    std::cerr << "Texture::Error::unsupported channel count " << channels
              << std::endl;
    stbi_image_free(data);
    width_ = height_ = nrChannels_ = 0;
    return false;
  }

  destroy();

  width_ = width;
  height_ = height;
  nrChannels_ = channels;

  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);

  // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width_, height_, 0, format,
               GL_UNSIGNED_BYTE, data);
  // glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glGenerateMipmap(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, 0);
  stbi_image_free(data);

  std::cout << "Texture::Loaded::" << pathString << " (" << width_ << "x"
            << height_ << ", " << nrChannels_ << " channels)" << std::endl;
  return true;
}

void Texture::destroy() {
  if (textureID != 0) {
    glDeleteTextures(1, &textureID);
    textureID = 0;
  }
  width_ = 0;
  height_ = 0;
  nrChannels_ = 0;
}

void Texture::bind(unsigned int unit) const {
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::unbind() const { glBindTexture(GL_TEXTURE_2D, 0); }
