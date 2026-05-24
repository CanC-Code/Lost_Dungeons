#version 300 es
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec3 a_Normal;

uniform mat4 u_MVP;
uniform mat4 u_Model;
uniform float u_Time;

out vec4 v_Color;
out vec3 v_Normal;
out vec3 v_FragPos;

void main() {
    vec3 pos = a_Position;
    
    // PROCEDURAL WIND: 
    // If the vertex is above the ground (y > 0.1), sway it using sine waves.
    // This makes grass tips and monster tops sway in the breeze.
    if (pos.y > 0.1) {
        float windStrength = 0.05 * pos.y; 
        pos.x += sin(u_Time * 2.0 + pos.z) * windStrength;
        pos.z += cos(u_Time * 1.5 + pos.x) * windStrength;
    }

    gl_Position = u_MVP * vec4(pos, 1.0);
    
    // Pass world position and normals to fragment shader for lighting
    v_FragPos = vec3(u_Model * vec4(pos, 1.0));
    v_Normal = a_Normal; 
    v_Color = a_Color;
}
