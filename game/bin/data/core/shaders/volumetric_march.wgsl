// Marches every view ray through a sphere of dust around the camera's target, asking the shadow map
// at each step whether that point can see the directional light. Rendered at a fraction of the
// window's resolution by VolumetricRenderPass; volumetric_composite.wgsl blends the result over the
// scene. Outputs the light scattered towards the camera in rgb, and the transmittance in alpha.

#include "/shaders/pandora/globals.wgsl"
#include "/shaders/pandora/shadows.wgsl"
#include "/shaders/pandora/fullscreen_triangle.wgsl"

struct Settings
{
    inverseViewProjection: mat4x4<f32>,
    volumeCenter: vec3f,
    volumeRadius: f32,
    wind: vec3f,
    extinction: f32, // Per world unit.
    noiseScales: vec2f, // Of the large and the small noise, in noise tiles per world unit.
    edgeFalloff: f32, // In world units.
    clumpiness: f32,
    anisotropy: f32, // Henyey-Greenstein g.
    scatterIntensity: f32,
    ambientIntensity: f32,
    stepCount: f32,
    jitter: f32, // 0 or 1.
    frame: f32,
    _padding: vec2f
};

@group(1) @binding(0) var<uniform> settings: Settings;
@group(1) @binding(1) var sceneDepth: texture_depth_multisampled_2d;
@group(1) @binding(2) var noiseTexture: texture_3d<f32>;
@group(1) @binding(3) var noiseSampler: sampler;

// The near and far distances along the ray at which it crosses the sphere; far < near if it misses.
fn raySphere(origin: vec3f, direction: vec3f) -> vec2f
{
    let oc = origin - settings.volumeCenter;
    let b = dot(oc, direction);
    let h = b * b - dot(oc, oc) + settings.volumeRadius * settings.volumeRadius;
    if (h < 0.0)
    {
        return vec2f(1.0, 0.0);
    }
    let s = sqrt(h);
    return vec2f(-b - s, -b + s);
}

fn henyeyGreenstein(cosTheta: f32, g: f32) -> f32
{
    let g2 = g * g;
    return (1.0 - g2) / (12.5663706 * pow(max(1.0 + g2 - 2.0 * g * cosTheta, 1e-4), 1.5));
}

// Jimenez's interleaved gradient noise: a per-pixel offset whose neighbours differ as much as possible.
fn interleavedGradientNoise(pixel: vec2f) -> f32
{
    return fract(52.9829189 * fract(dot(pixel, vec2f(0.06711056, 0.00583715))));
}

// Extinction at a point: two octaves of drifting noise, cut into clumps and faded out towards the
// sphere's edge.
fn density(position: vec3f) -> f32
{
    let drift = settings.wind * uGlobalUniforms.time;
    let large = textureSampleLevel(noiseTexture, noiseSampler, position * settings.noiseScales.x + drift, 0.0).r;
    let small = textureSampleLevel(noiseTexture, noiseSampler, position * settings.noiseScales.y + drift * 2.3, 0.0).r;
    let coverage = smoothstep(0.35, 0.78, large * 0.72 + small * 0.38);
    let edge = smoothstep(0.0, settings.edgeFalloff, settings.volumeRadius - length(position - settings.volumeCenter));
    return mix(1.0, coverage * 2.0, settings.clumpiness) * edge * settings.extinction;
}

@fragment fn marchMain(in: VertexOutput) -> @location(0) vec4f
{
    // The scene's depth at the full resolution texel under this one's centre. Sample 0 stands in
    // for the whole pixel.
    let depthSize = textureDimensions(sceneDepth);
    let depthTexel = min(vec2u(in.uv * vec2f(depthSize)), depthSize - 1u);
    let depth = textureLoad(sceneDepth, depthTexel, 0);

    let clip = vec4f(in.uv.x * 2.0 - 1.0, 1.0 - in.uv.y * 2.0, depth, 1.0);
    let world = settings.inverseViewProjection * clip;
    let cameraPosition = uGlobalUniforms.cameraPosition.xyz;
    let toPixel = world.xyz / world.w - cameraPosition;
    let rayDirection = normalize(toPixel);
    let sceneDistance = select(length(toPixel), 1e9, depth >= 1.0);

    let crossing = raySphere(cameraPosition, rayDirection);
    let start = max(crossing.x, 0.0);
    let end = min(crossing.y, sceneDistance);
    if (end <= start)
    {
        return vec4f(0.0, 0.0, 0.0, 1.0);
    }

    let stepLength = (end - start) / settings.stepCount;
    let offset = select(0.5, interleavedGradientNoise(in.position.xy + 5.588238 * settings.frame), settings.jitter > 0.5);
    let toLight = normalize(uGlobalUniforms.directionalLightDirection.xyz);
    let phase = henyeyGreenstein(dot(rayDirection, toLight), settings.anisotropy);
    let sun = uGlobalUniforms.directionalLightColor.rgb * settings.scatterIntensity * phase;
    // The SH's constant band: the ambient light averaged over every direction.
    let ambient = uGlobalUniforms.ambientSH[0].rgb * 0.282095 * settings.ambientIntensity;

    var scattered = vec3f(0.0);
    var transmittance = 1.0;
    for (var i = 0u; i < u32(settings.stepCount); i++)
    {
        let position = cameraPosition + rayDirection * (start + (f32(i) + offset) * stepLength);
        let sigma = density(position);
        if (sigma < 1e-6)
        {
            continue;
        }

        let incoming = sun * sampleShadowAt(position) + ambient;
        let stepTransmittance = exp(-sigma * stepLength);
        // The step's integral, with an albedo of 1: what it scatters is what it takes out.
        scattered += transmittance * incoming * (1.0 - stepTransmittance);
        transmittance *= stepTransmittance;
        if (transmittance < 0.01)
        {
            break;
        }
    }
    return vec4f(scattered, transmittance);
}
