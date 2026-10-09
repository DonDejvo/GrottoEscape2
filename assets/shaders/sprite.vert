#version 300 es

layout(location=0) in vec2 aPosition;
layout(location=1) in vec2 aUv;

layout(std140) uniform Camera {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec3 color;
} camera;

out vec2 uv;

void main() {
    uv = aUv;
    gl_Position = camera.projectionMatrix * camera.viewMatrix * vec4(aPosition, 0, 1);
}