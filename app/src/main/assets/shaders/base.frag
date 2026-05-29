#version 300 es
precision mediump float;

in vec4 v_Color;
in float v_FogFactor;

// Driven by the day/night cycle via JNI bridge
uniform vec3 u_SkyColorBottom; 

out vec4 FragColor;

void main() {
    // Dynamically melt the rendered terrain and entities into the horizon 
    vec3 finalColor = mix(u_SkyColorBottom, v_Color.rgb, v_FogFactor);
    FragColor = vec4(finalColor, v_Color.a);
}
