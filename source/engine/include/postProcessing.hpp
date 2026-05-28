#ifndef POST_PROCESSING_HPP
#define POST_PROCESSING_HPP

#include "shader.hpp"

struct GodRayParams {
    Vector2 screenPos;      // позиция на экране [0,1]
    Vector3 color;          // цвет лучей
    float exposure;         // яркость
    float decay;            // затухание
    float density;          // плотность сэмплов
    float weight;           // вес
    float size;             // размер источника
};

class PostProcess {
public:
    PostProcess(int width, int height);
    ~PostProcess();

    bool init();
    void beginCapture();
    void endCapture();
    void renderGodRays(unsigned int sourceTexture, const GodRayParams& params);
    void renderGodRaysMulti(const std::vector<GodRayParams>& params);
    void renderToScreen(); 

    unsigned int getColorTexture() const { return colorTexture; }

private:
    int width, height;
    unsigned int fbo;
    unsigned int colorTexture;
    unsigned int tempFbo;
    unsigned int tempTexture;
    Shader godRaysShader;
    Shader blendShader;
    unsigned int quadVAO, quadVBO;
    void createQuad();
};

#endif