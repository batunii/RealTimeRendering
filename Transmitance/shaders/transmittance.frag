#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 ReflectDir;

uniform sampler2D skyboxTexture;  // Your skybox texture
uniform float reflectivity;       // 0.0 to 1.0

// Convert 3D direction to 2D UV for cross-layout skybox
vec2 directionToUV(vec3 dir)
{
    vec3 absDir = abs(dir);
    vec2 uv;
    float faceIndex;
    
    // Determine which face of the cube to use
    if (absDir.x >= absDir.y && absDir.x >= absDir.z) {
        // X-axis dominant
        if (dir.x > 0.0) {
            // Right face
            uv = vec2(-dir.z, -dir.y) / absDir.x;
            faceIndex = 0.0;
        } else {
            // Left face
            uv = vec2(dir.z, -dir.y) / absDir.x;
            faceIndex = 1.0;
        }
    } else if (absDir.y >= absDir.x && absDir.y >= absDir.z) {
        // Y-axis dominant
        if (dir.y > 0.0) {
            // Top face
            uv = vec2(dir.x, dir.z) / absDir.y;
            faceIndex = 2.0;
        } else {
            // Bottom face
            uv = vec2(dir.x, -dir.z) / absDir.y;
            faceIndex = 3.0;
        }
    } else {
        // Z-axis dominant
        if (dir.z > 0.0) {
            // Front face
            uv = vec2(dir.x, -dir.y) / absDir.z;
            faceIndex = 4.0;
        } else {
            // Back face
            uv = vec2(-dir.x, -dir.y) / absDir.z;
            faceIndex = 5.0;
        }
    }
    
    // Convert from [-1, 1] to [0, 1]
    uv = uv * 0.5 + 0.5;
    
    // Map to cross layout positions
    // Cross layout: 4 wide, 3 tall
    vec2 faceUV;
    if (faceIndex == 0.0) {
        // Right
        faceUV = vec2(2.0, 1.0);
    } else if (faceIndex == 1.0) {
        // Left
        faceUV = vec2(0.0, 1.0);
    } else if (faceIndex == 2.0) {
        // Top
        faceUV = vec2(1.0, 0.0);
    } else if (faceIndex == 3.0) {
        // Bottom
        faceUV = vec2(1.0, 2.0);
    } else if (faceIndex == 4.0) {
        // Front
        faceUV = vec2(1.0, 1.0);
    } else {
        // Back
        faceUV = vec2(3.0, 1.0);
    }
    
    // Scale UV to fit in 1/4 x 1/3 of texture
    vec2 finalUV = (faceUV + uv) / vec2(4.0, 3.0);
    
    return finalUV;
}

void main()
{
    // Sample skybox using reflection direction
    vec2 uv = directionToUV(normalize(ReflectDir));
    vec3 reflection = texture(skyboxTexture, uv).rgb;
    
    FragColor = vec4(reflection * reflectivity, 1.0);
}
