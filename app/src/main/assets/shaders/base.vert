#version 300 es
layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec4 a_Color;

uniform mat4 u_MVP; // NEW: Model-View-Projection Matrix

out vec4 v_Color;

void main() {
    // Apply camera transformations to the vertices
    gl_Position = u_MVP * a_Position;
    v_Color = a_Color;
}
