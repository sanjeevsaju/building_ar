#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require
precision mediump float;
in vec2 v_TexCoord;
layout(location = 0) out vec4 outColor;
uniform samplerExternalOES u_Texture;

void main() {
    outColor = texture(u_Texture, v_TexCoord);
}