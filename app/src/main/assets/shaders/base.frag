#version 300 es
precision mediump float;

in vec4 v_Color;
in vec3 v_Normal;
in vec3 v_FragPos;

uniform vec3 u_LightDir;     // Direction of Sun/Moon
uniform vec3 u_LightColor;   // Color of Sun/Moon
uniform vec3 u_AmbientColor; // Base environment light
uniform vec3 u_SkyColorTop;
uniform vec3 u_SkyColorBottom;

out vec4 fragColor;

void main() {
    // 1. Procedural Skybox (Rendered if geometry is far away)
    if (v_FragPos.y > 10.0) {
        fragColor = vec4(mix(u_SkyColorBottom, u_SkyColorTop, clamp(v_FragPos.y/50.0, 0.0, 1.0)), 1.0);
        return;
    }

    // 2. Base Lighting Math (Lambertian Reflectance)
    vec3 ambient = u_AmbientColor * v_Color.rgb;
    
    vec3 norm = normalize(v_Normal);
    vec3 lightDir = normalize(-u_LightDir); 
    
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * u_LightColor * v_Color.rgb;

    vec3 result = ambient + diffuse;
    fragColor = vec4(result, v_Color.a);
}
