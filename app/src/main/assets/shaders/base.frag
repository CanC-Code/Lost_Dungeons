#version 300 es
precision mediump float;

in vec4 v_Color;
in vec3 v_Normal;
in vec3 v_FragPos;

uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientColor;
uniform vec3 u_SkyColorTop;
uniform vec3 u_SkyColorBottom;
uniform vec3 u_CameraPos; // NEW: Needed for fog math

out vec4 fragColor;

void main() {
    // 1. Base Lighting
    vec3 ambient = u_AmbientColor * v_Color.rgb;
    vec3 norm = normalize(v_Normal);
    vec3 lightDir = normalize(-u_LightDir); 
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor * v_Color.rgb;
    vec3 resultColor = ambient + diffuse;

    // 2. Atmospheric Distance Fog
    // Calculate distance from camera to this specific pixel
    float dist = length(v_FragPos - u_CameraPos);
    
    // Fog starts at 5.0 units away and fully obscures at 25.0 units
    float fogFactor = clamp((dist - 5.0) / 20.0, 0.0, 1.0);
    
    // Smoothly mix the terrain color with the bottom sky color
    vec3 finalColor = mix(resultColor, u_SkyColorBottom, fogFactor);

    fragColor = vec4(finalColor, v_Color.a);
}
