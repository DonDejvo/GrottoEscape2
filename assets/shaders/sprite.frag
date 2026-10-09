#version 300 es

precision highp float;

in vec2 uv;

layout(std140) uniform Camera {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec3 color;
} camera;

uniform sampler2D spriteTexture;

out vec4 fragColor;

void main() {
    fragColor = vec4(camera.color, 1.0) * texture(spriteTexture, uv);
}