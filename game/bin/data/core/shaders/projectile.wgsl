// Draws each projectile as a quad stretched along its flight path and turned about it to face the
// camera. Additive and linear HDR, so bright tracers bloom.
@group(0) @binding(0) var<uniform> uGlobalUniforms: GlobalUniforms;

struct InstanceInput
{
    @location(0) startAndLength: vec4f,
    @location(1) directionAndWidth: vec4f,
    @location(2) color: vec4f
};

struct VertexOutput
{
    @builtin(position) position: vec4f,
    @location(0) uv: vec2f, // x: -1 to 1 across the tracer, y: 0 at the tail to 1 at the head.
    @location(1) color: vec3f
};

@vertex fn vertexMain(@builtin(vertex_index) vertexIndex: u32, instance: InstanceInput) -> VertexOutput
{
    var corners = array<vec2f, 6>(
        vec2f(-1.0, 0.0), vec2f(1.0, 0.0), vec2f(1.0, 1.0),
        vec2f(-1.0, 0.0), vec2f(1.0, 1.0), vec2f(-1.0, 1.0));
    let corner = corners[vertexIndex];

    let start = instance.startAndLength.xyz;
    let direction = instance.directionAndWidth.xyz;
    let toCamera = uGlobalUniforms.cameraPosition.xyz - start;
    let side = normalize(cross(direction, toCamera)) * instance.directionAndWidth.w * 0.5;
    let worldPosition = start + direction * instance.startAndLength.w * corner.y + side * corner.x;

    var out: VertexOutput;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * vec4f(worldPosition, 1.0);
    out.uv = corner;
    out.color = instance.color.rgb;
    return out;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f
{
    // A soft core across the width, fading out towards the tail.
    let intensity = exp(-4.0 * in.uv.x * in.uv.x) * in.uv.y * in.uv.y;
    return vec4f(in.color * intensity, 0.0);
}
