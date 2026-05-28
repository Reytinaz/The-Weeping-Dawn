#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D depthTexture;
uniform mat4 lightSpaceMatrix;
uniform vec3 cameraPos;
uniform vec3 lightDir;

void main() {
    // Пока просто рисуем красный цвет (для теста)
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}