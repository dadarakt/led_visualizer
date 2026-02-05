#version 330 core

// Multiple render targets for G-Buffer
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedo;

// Input from vertex shader
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

// Texture and color
uniform sampler2D texture0;
uniform vec4 colDiffuse;

void main() {
    gPosition = fragPosition;
    gNormal = normalize(fragNormal);

    vec4 texColor = texture(texture0, fragTexCoord);
    gAlbedo = texColor * colDiffuse * fragColor;
}
