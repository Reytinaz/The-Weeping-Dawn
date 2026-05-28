#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D diffuseTexture;
uniform samplerCube skybox;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform float ambientStrength;
uniform float specularStrength;
uniform float shininess;
uniform float reflectivity;

// CSM
uniform sampler2DArray shadowMap;
uniform mat4 lightSpaceMatrices[4];
uniform float cascadeSplits[5];
uniform int numCascades;
uniform float bias;
uniform float lightDir[3]; // направление солнца

#define MAX_LIGHTS 8
struct Light {
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float cutOff;
    float outerCutOff;
};
uniform Light lights[MAX_LIGHTS];
uniform int numLights;

float ShadowCalculation(vec3 worldPos, vec3 norm, vec3 lightDir) {
    float depth = distance(viewPos, worldPos);
    
    int cascadeIndex = 0;
    for (int i = 0; i < numCascades - 1; i++) {
        if (depth > cascadeSplits[i + 1]) cascadeIndex = i + 1;
    }
    
    vec4 fragPosLightSpace = lightSpaceMatrices[cascadeIndex] * vec4(worldPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || 
        projCoords.y < 0.0 || projCoords.y > 1.0 || 
        projCoords.z > 1.0) return 1.0;
    
    float bias = 0.005 * tan(acos(max(dot(norm, lightDir), 0.001)));
    bias = clamp(bias, 0.0, 0.01);
    
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0).xy;
    int samples = 2; // moderate PCF sampling count
    float radius = (cascadeIndex == 0) ? 1.5 : 1.0;
    
    for (int x = -samples; x <= samples; x++) {
        for (int y = -samples; y <= samples; y++) {
            vec2 offset = vec2(x, y) * texelSize * radius;
            float closestDepth = texture(shadowMap, vec3(projCoords.xy + offset, cascadeIndex)).r;
            shadow += (projCoords.z - bias) > closestDepth ? 1.0 : 0.0;
        }
    }
    
    shadow /= ((2*samples+1)*(2*samples+1));
    
    if (cascadeIndex < numCascades - 1) {
        float blendDist = 0.2 * (cascadeSplits[cascadeIndex + 1] - cascadeSplits[cascadeIndex]);
        float blend = clamp((cascadeSplits[cascadeIndex + 1] - depth) / blendDist, 0.0, 1.0);
        
        if (blend < 1.0) {
            vec4 fragPosLightSpace2 = lightSpaceMatrices[cascadeIndex + 1] * vec4(worldPos, 1.0);
            vec3 projCoords2 = fragPosLightSpace2.xyz / fragPosLightSpace2.w;
            projCoords2 = projCoords2 * 0.5 + 0.5;
            
            float shadow2 = 0.0;
            for (int x = -samples; x <= samples; x++) {
                for (int y = -samples; y <= samples; y++) {
                    vec2 offset = vec2(x, y) * texelSize * radius;
                    float closestDepth = texture(shadowMap, vec3(projCoords2.xy + offset, cascadeIndex + 1)).r;
                    shadow2 += (projCoords2.z - bias) > closestDepth ? 1.0 : 0.0;
                }
            }
            shadow2 /= ((2*samples+1)*(2*samples+1));
            shadow = mix(shadow, shadow2, 1.0 - blend);
        }
    }
    
    return 1.0 - shadow;
}

void main() {
    vec4 texColor = texture(diffuseTexture, TexCoord);
    if (texColor.a < 0.1) discard;
    float materialRough = 0.5;

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    vec3 sunDir = normalize(-vec3(lightDir[0], lightDir[1], lightDir[2]));
    float sunShadow = ShadowCalculation(FragPos, norm, sunDir);
    
    vec3 result = vec3(0.0);
    
    for (int i = 0; i < numLights; i++) {
        vec3 lightDirVec;
        float attenuation = 1.0;
        
        if (lights[i].type == 0) { // directional
            lightDirVec = normalize(-lights[i].direction);
        } else {
            vec3 toLight = lights[i].position - FragPos;
            float dist = length(toLight);
            lightDirVec = toLight / dist;
            attenuation = 1.0 / (lights[i].constant + 
                                  lights[i].linear * dist + 
                                  lights[i].quadratic * dist * dist);
            
            if (lights[i].type == 2) { // spot
                float theta = dot(lightDirVec, normalize(-lights[i].direction));
                float epsilon = lights[i].cutOff - lights[i].outerCutOff;
                float spotIntensity = clamp((theta - lights[i].outerCutOff) / epsilon, 0.0, 1.0);
                attenuation *= spotIntensity;
            }
        }
        
        float diff = max(dot(norm, lightDirVec), 0.0);
        vec3 diffuse = diff * lights[i].color * lights[i].intensity * attenuation;

        vec3 halfDir = normalize(lightDirVec + viewDir);
        float specTerm = pow(max(dot(norm, halfDir), 0.0), shininess);
        // attenuate specular by squared diffuse to avoid extreme hotspots on sharp normals
        float specMod = diff * diff;
        vec3 specular = specularStrength * specTerm * specMod * lights[i].color * lights[i].intensity * attenuation;
        // clamp specular to avoid excessive values (energy-conserving approximation)
        specular = clamp(specular, vec3(0.0), vec3(1.0));
        
        if (i == 0 && lights[i].type == 0) {
            result += (diffuse + specular) * sunShadow;
        } else {
            result += (diffuse + specular);
        }
    }
    
    vec3 ambient = ambientStrength * vec3(1.0);
    result = (ambient + result) * texColor.rgb * objectColor;
    
    if (reflectivity > 0.0) {
        vec3 incident = normalize(FragPos - viewPos);
        vec3 reflection = reflect(incident, norm);
        vec3 skyReflection = texture(skybox, reflection).rgb;
        result = mix(result, skyReflection, reflectivity);
    }
    
    FragColor = vec4(result, texColor.a);
}