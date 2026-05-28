#include "texture.hpp"
#include "glad.h"
#include "SFML/Graphics/Image.hpp"

Texture::Texture() : id(0), width(0), height(0), channels(0) {}

Texture::~Texture() {
    release();
}

bool Texture::loadFromFile(const std::string& filename, bool generateMipmaps) {
    release();

    sf::Image img;
    if (!img.loadFromFile(filename)) {
        std::cerr << "Failed to load texture: " << filename << std::endl;
        return false;
    }

    width = img.getSize().x;
    height = img.getSize().y;
    GLenum format = GL_RGBA;
    GLenum internalFormat = GL_SRGB8_ALPHA8;
    GLfloat maxAniso;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, generateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, img.getPixelsPtr());
    if (generateMipmaps) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    return true;
}

void Texture::bind(unsigned int unit) const {
    if (id) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, id);
    }
}

void Texture::release() {
    if (id) {
        glDeleteTextures(1, &id);
        id = 0;
    }
}

void Texture::setWrapMode(unsigned int wrapS, unsigned int wrapT) const {
    if (id) {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    }
}

void Texture::setFilter(unsigned int minFilter, unsigned int magFilter) const {
    if (id) {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    }
}