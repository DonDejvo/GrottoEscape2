struct VertexInput {
    @location(0) position: vec2f,
    @location(1) uv: vec2f
}

struct VertexOutput {
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f
}

struct Camera {
    viewMatrix: mat4x4f,
    projectionMatrix: mat4x4f,
    color: vec3f
}

@group(0) @binding(0) 
var<uniform> camera: Camera;

@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    
    output.position = camera.projectionMatrix * camera.viewMatrix * vec4f(input.position, 0.0, 1.0);
    output.uv = input.uv;

    return output;
}

@group(1) @binding(0) 
var spriteTexture: texture_2d<f32>;
@group(1) @binding(1) 
var spriteSampler: sampler;

@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4f {
    return vec4f(camera.color, 1.0) * textureSample(spriteTexture, spriteSampler, input.uv);
}
