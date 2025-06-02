#version 300 es
precision mediump float;

in vec2 vTexCoord;
in vec3 vNormal;

uniform sampler2D textureSampler;

out vec4 fragColor;

void main() {
//    vec4 texColor = texture(textureSampler, vTexCoord);

    // Simple lightning
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normalize(vNormal), lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

//    fragColor = vec4(texColor.rgb * (0.3 + diffuse * 0.7), texColor.a);
    fragColor = texture(textureSampler, vTexCoord);
}