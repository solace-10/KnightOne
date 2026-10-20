// Upsamples volumetric_march.wgsl's output to the window's resolution and blends it over the
// scene. The blend state works out scene * transmittance + scattered, so this only writes the two.

#include "/shaders/pandora/fullscreen_triangle.wgsl"

struct Settings
{
    nearPlane: f32,
    farPlane: f32,
    // 0: composite, 1: scattered light only, 2: transmittance only.
    mode: u32
};

@group(0) @binding(0) var<uniform> settings: Settings;
@group(0) @binding(1) var volumeTexture: texture_2d<f32>;
@group(0) @binding(2) var sceneDepth: texture_depth_multisampled_2d;

fn linearDepth(texel: vec2u) -> f32
{
    let depth = textureLoad(sceneDepth, texel, 0);
    return settings.nearPlane * settings.farPlane / (settings.farPlane - depth * (settings.farPlane - settings.nearPlane));
}

// Bilinear over the four nearest marched texels, each weighted down the further its depth is from
// this pixel's, so that the haze of the background doesn't bleed over a ship's silhouette.
fn upsample(uv: vec2f, pixel: vec2u) -> vec4f
{
    let volumeSize = textureDimensions(volumeTexture);
    let depthSize = vec2f(textureDimensions(sceneDepth));
    let position = uv * vec2f(volumeSize) - 0.5;
    let base = vec2i(floor(position));
    let f = fract(position);
    let pixelDepth = linearDepth(pixel);

    var sum = vec4f(0.0);
    var weightSum = 0.0;
    for (var y = 0; y < 2; y++)
    {
        for (var x = 0; x < 2; x++)
        {
            let texel = vec2u(clamp(base + vec2i(x, y), vec2i(0), vec2i(volumeSize) - 1));
            // The same full resolution texel the march read its depth from.
            let depthTexel = min(vec2u((vec2f(texel) + 0.5) / vec2f(volumeSize) * depthSize), vec2u(depthSize) - 1u);
            let bilinear = select(1.0 - f.x, f.x, x == 1) * select(1.0 - f.y, f.y, y == 1);
            let depthWeight = 1.0 / (0.002 + abs(pixelDepth - linearDepth(depthTexel)) / max(pixelDepth, 1.0));
            let weight = bilinear * depthWeight + 1e-6;
            sum += textureLoad(volumeTexture, texel, 0) * weight;
            weightSum += weight;
        }
    }
    return sum / weightSum;
}

@fragment fn compositeMain(in: VertexOutput) -> @location(0) vec4f
{
    let volume = upsample(in.uv, vec2u(in.position.xy));
    switch settings.mode
    {
        case 1u: { return vec4f(volume.rgb, 0.0); }
        case 2u: { return vec4f(vec3f(volume.a), 0.0); }
        default: { return volume; }
    }
}
