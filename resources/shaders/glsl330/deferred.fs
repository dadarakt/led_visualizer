#version 330 core

out vec4 finalColor;

in vec2 fragTexCoord;

// G-Buffer textures
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;

// Light data texture: each row is one light
// Row format: pixel 0 = (posX, posY, posZ, intensity), pixel 1 = (r, g, b, enabled)
uniform sampler2D lightData;
uniform int numLights;
uniform int lightTexWidth;

uniform vec3 viewPos;
uniform vec4 ambient;

// Fog parameters
const vec3 fogColor = vec3(0.12, 0.12, 0.14);
const float fogDensity = 0.35;

void main() {
    vec3 fragPosition = texture(gPosition, fragTexCoord).rgb;
    vec3 normal = texture(gNormal, fragTexCoord).rgb;
    vec4 albedo = texture(gAlbedo, fragTexCoord);

    // Early out for background pixels (no geometry)
    if (length(normal) < 0.1) {
        finalColor = vec4(fogColor, 1.0);
        return;
    }

    vec3 viewDir = normalize(viewPos - fragPosition);
    float viewDist = length(viewPos - fragPosition);
    vec3 lighting = vec3(0.0);
    vec3 specular = vec3(0.0);
    vec3 fogScatter = vec3(0.0);

    float texelSize = 1.0 / float(lightTexWidth);

    for (int i = 0; i < numLights; i++) {
        // Each light uses 2 texels: (pos+intensity), (color+enabled)
        float u0 = (float(i * 2) + 0.5) * texelSize;
        float u1 = (float(i * 2 + 1) + 0.5) * texelSize;

        vec4 posIntensity = texture(lightData, vec2(u0, 0.5));
        vec4 colorEnabled = texture(lightData, vec2(u1, 0.5));

        if (colorEnabled.a < 0.5) continue; // light disabled

        vec3 lightPos = posIntensity.xyz;
        float intensity = posIntensity.w;
        vec3 lightColor = colorEnabled.rgb;

        // Surface lighting
        vec3 lightDir = normalize(lightPos - fragPosition);
        float dist = length(lightPos - fragPosition);
        float attenuation = intensity / (1.0 + dist * dist);

        // Diffuse
        float NdotL = max(dot(normal, lightDir), 0.0);
        lighting += lightColor * NdotL * attenuation;

        // Specular (Blinn-Phong)
        if (NdotL > 0.0) {
            vec3 halfDir = normalize(lightDir + viewDir);
            float spec = pow(max(dot(normal, halfDir), 0.0), 16.0);
            specular += lightColor * spec * attenuation;
        }

        // Volumetric fog: scatter light based on how close it is to the view ray
        // and how far along the ray from camera to fragment
        vec3 camToFrag = fragPosition - viewPos;
        vec3 camToLight = lightPos - viewPos;

        // Project light position onto the view ray
        float t = clamp(dot(camToLight, camToFrag) / dot(camToFrag, camToFrag), 0.0, 1.0);
        vec3 closestPoint = viewPos + t * camToFrag;
        float lightToRayDist = length(lightPos - closestPoint);

        // Scatter contribution - closer lights scatter more
        float scatter = intensity * 0.5 / (0.1 + lightToRayDist * lightToRayDist);
        fogScatter += lightColor * scatter;
    }

    finalColor = albedo * vec4(lighting + specular, 1.0);
    finalColor += albedo * (ambient / 10.0);

    // Gamma correction
    finalColor = pow(finalColor, vec4(1.0 / 2.2));

    // Exponential fog with light scattering
    float fogFactor = exp(-fogDensity * viewDist);
    vec3 fogWithLight = fogColor + fogScatter;
    finalColor.rgb = mix(fogWithLight, finalColor.rgb, fogFactor);
    finalColor.a = 1.0;
}
