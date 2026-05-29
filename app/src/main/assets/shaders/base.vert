#version 300 es
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec3 a_Normal;

uniform mat4 u_MVP;
uniform mat4 u_Model;
uniform vec3 u_CameraPos;

out vec4 v_Color;
out float v_FogFactor;

void main() {
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    gl_Position = u_MVP * worldPos;
    v_Color = a_Color;

    // Density recalibrated to seamlessly shroud edges of the new 1000m render bounds
    float distance = length(u_CameraPos.xz - worldPos.xz);
    float fogDensity = 0.0025; 
    
    v_FogFactor = exp(-pow((distance * fogDensity), 2.0));
    v_FogFactor = clamp(v_FogFactor, 0.0, 1.0);
}
