#version 300 es
layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
uniform mat4 mvp;
void main() {
    gl_Position = mvp * vec4(vPosition, 1.0);
    vTexCoord = aTexCoord;
}