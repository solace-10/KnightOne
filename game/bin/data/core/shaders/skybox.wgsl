// Draws the baked sky behind the scene, plus stars at screen resolution.
struct SkyUniforms
{
    params: vec4f // x: star intensity, y: 1 to draw the stars
};

#include "/shaders/pandora/globals.wgsl"
@group(1) @binding(0) var skyTexture: texture_cube<f32>;
@group(1) @binding(1) var skySampler: sampler;
@group(1) @binding(2) var<uniform> uSky: SkyUniforms;

struct VertexOutput
{
    @builtin(position) position: vec4f,
    @location(0) ndc: vec2f
};

@vertex fn vertexMain(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput
{
    let xy = vec2f(f32((vertexIndex << 1u) & 2u), f32(vertexIndex & 2u)) * 2.0 - 1.0;
    var out: VertexOutput;
    out.position = vec4f(xy, 1.0, 1.0);
    out.ndc = xy;
    return out;
}

#include "/shaders/common/hash.wgsl"

// Three layers of stars on a cube-projected grid, each star a Gaussian about a pixel wide.
fn stars(d: vec3f, pixelAngle: f32) -> vec3f
{
    let a = abs(d);
    var uv: vec2f;
    var face: f32;
    if (a.x >= a.y && a.x >= a.z)
    {
        uv = d.yz / a.x;
        face = select(1.0, 0.0, d.x > 0.0);
    }
    else if (a.y >= a.z)
    {
        uv = d.xz / a.y;
        face = select(3.0, 2.0, d.y > 0.0);
    }
    else
    {
        uv = d.xy / a.z;
        face = select(5.0, 4.0, d.z > 0.0);
    }

    // A pixel covers more of the projected grid towards the face's corners.
    let pixelUv = pixelAngle * (1.0 + dot(uv, uv));
    let radius = pixelUv * 0.8;

    var scales = array<f32, 3>(30.0, 90.0, 180.0);
    var probabilities = array<f32, 3>(0.25, 0.18, 0.12);
    var gains = array<f32, 3>(7.0, 1.8, 0.7);

    var sum = vec3f(0.0);
    for (var layer = 0u; layer < 3u; layer++)
    {
        let scale = scales[layer];
        let g = uv * scale;
        let cell = floor(g);
        let f = g - cell;
        let h = hash33(vec3f(cell, face * 37.0 + f32(layer) * 113.0));
        if (h.z > probabilities[layer])
        {
            continue;
        }

        let h2 = hash33(vec3f(cell.yx + 7.3, face * 11.0 + f32(layer) * 57.0 + 3.1));
        let offset = (f - (0.2 + 0.6 * h.xy)) / scale;
        let d2 = dot(offset, offset);
        var brightness = exp(-d2 / (radius * radius));
        if (layer == 0u)
        {
            brightness += 0.04 * exp(-d2 / (radius * radius * 20.0));
        }
        let tint = mix(vec3f(0.65, 0.78, 1.25), vec3f(1.25, 0.92, 0.62), h2.y);
        sum += mix(tint, vec3f(1.0), 0.4) * brightness * pow(h2.x, 3.0) * gains[layer];
    }
    return sum;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f
{
    let projection = uGlobalUniforms.projectionMatrix;
    let view = uGlobalUniforms.viewMatrix;

    // The view matrix's rotation is orthonormal, so its transpose takes the ray back to world space.
    let viewRay = vec3f(in.ndc.x / projection[0][0], in.ndc.y / projection[1][1], -1.0);
    let viewRotation = mat3x3f(view[0].xyz, view[1].xyz, view[2].xyz);
    let direction = normalize(transpose(viewRotation) * viewRay);

    var color = textureSample(skyTexture, skySampler, direction).rgb;

    if (uSky.params.y > 0.5)
    {
        // Stars are thinned out where the nebula is bright, as they would be lost in it anyway.
        let pixelAngle = 2.0 / (projection[1][1] * uGlobalUniforms.windowHeight);
        let occlusion = 1.0 / (1.0 + dot(color, vec3f(0.2126, 0.7152, 0.0722)) * 4.0);
        color += stars(direction, pixelAngle) * uSky.params.x * occlusion;
    }

    return vec4f(color, 1.0);
}
