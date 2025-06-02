#version 300 es
precision mediump float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

uniform mat4 mvp;

out vec2 vTexCoord;
out vec3 vNormal;

void main() {
    gl_Position = mvp * vec4(aPosition, 1.0);
    vTexCoord = aTexCoord;
    vNormal = aNormal;
}