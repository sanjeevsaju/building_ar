#version 300 es
precision mediump float;

in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uTexture;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);
    if (texColor.a < 0.1) discard;

    /* Simple lighting */
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(normal, lightDir), 0.3);
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);

    fragColor = vec4(texColor.rgb * (0.7 + 0.3 * diffuse), texColor.a);

//    fragColor = texColor;
}
