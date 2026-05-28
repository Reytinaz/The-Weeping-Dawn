#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "string"
#include "iostream"
#include "memory"

class Texture {
public:
    Texture();
    ~Texture();

    bool loadFromFile(const std::string& filename, bool generateMipmaps = true);
    void bind(unsigned int unit = 0) const;
    void release();

    unsigned int getId() const { return id; }
    void setWrapMode(unsigned int wrapS, unsigned int wrapT) const;
    void setFilter(unsigned int minFilter, unsigned int magFilter) const;

private:
    unsigned int id;
    int width, height, channels;
};

#endif