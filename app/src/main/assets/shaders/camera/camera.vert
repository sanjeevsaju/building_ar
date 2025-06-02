#version 300 es
layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec2 a_TexCoord;
out vec2 v_TexCoord;
uniform int u_Rotation;

void main() {
    gl_Position = a_Position;
    switch(u_Rotation) {
        case 1: // 90° counter-clockwise (landscape)
            v_TexCoord = vec2(a_TexCoord.x, a_TexCoord.y);
            break;
        case 2: // 180° (upside down portrait)
            v_TexCoord = vec2(1.0 - a_TexCoord.y, a_TexCoord.x);
            break;
        case 3: // 270° counter-clockwise (reverse landscape)
            v_TexCoord = vec2(1.0 - a_TexCoord.x, 1.0 - a_TexCoord.y);
            break;
        default: // 0° (portrait)
            // Rotate 90° clockwise from landscape to portrait
            v_TexCoord = vec2(a_TexCoord.y, 1.0 - a_TexCoord.x);
    }
}