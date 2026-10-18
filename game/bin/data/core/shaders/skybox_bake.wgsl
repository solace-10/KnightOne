// Bakes the nebula into one cubemap face per instance. Mirrors SkyboxBaker::BakeUniforms.
struct BakeUniforms
{
    orientation: mat4x4<f32>,
    lobeZ: array<vec4f, 8>,
    lobeX: array<vec4f, 8>,
    lobeY: array<vec4f, 8>,
    lobeColor: array<vec4f, 8>,
    coreDirection: array<vec4f, 4>,
    coreColor: array<vec4f, 4>,
    floorColorA: vec4f,
    floorColorB: vec4f,
    absorb: vec4f,
    seed: vec4f,
    params: vec4f, // x: warp strength, y: noise scale, z: dust, w: floor glow
    counts: vec4u // x: lobe count, y: core count, z: stage, w: face size
};

const kStageWarp: u32 = 1u;
const kStageDust: u32 = 2u;

@group(0) @binding(0) var<uniform> uBake: BakeUniforms;

struct VertexOutput
{
    @builtin(position) position: vec4f,
    @location(0) @interpolate(flat) face: u32
};

@vertex fn vertexMain(@builtin(vertex_index) vertexIndex: u32, @builtin(instance_index) face: u32) -> VertexOutput
{
    let xy = vec2f(f32((vertexIndex << 1u) & 2u), f32(vertexIndex & 2u));
    var out: VertexOutput;
    out.position = vec4f(xy * 2.0 - 1.0, 0.0, 1.0);
    out.face = face;
    return out;
}

fn hash33(p: vec3f) -> vec3f
{
    var p3 = fract(p * vec3f(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yxx) * p3.zyx);
}

fn gradient(cell: vec3f) -> vec3f
{
    return hash33(cell) * 2.0 - 1.0;
}

fn gradientNoise(p: vec3f) -> f32
{
    let i = floor(p);
    let f = p - i;
    let u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    let n000 = dot(gradient(i), f);
    let n100 = dot(gradient(i + vec3f(1.0, 0.0, 0.0)), f - vec3f(1.0, 0.0, 0.0));
    let n010 = dot(gradient(i + vec3f(0.0, 1.0, 0.0)), f - vec3f(0.0, 1.0, 0.0));
    let n110 = dot(gradient(i + vec3f(1.0, 1.0, 0.0)), f - vec3f(1.0, 1.0, 0.0));
    let n001 = dot(gradient(i + vec3f(0.0, 0.0, 1.0)), f - vec3f(0.0, 0.0, 1.0));
    let n101 = dot(gradient(i + vec3f(1.0, 0.0, 1.0)), f - vec3f(1.0, 0.0, 1.0));
    let n011 = dot(gradient(i + vec3f(0.0, 1.0, 1.0)), f - vec3f(0.0, 1.0, 1.0));
    let n111 = dot(gradient(i + vec3f(1.0, 1.0, 1.0)), f - vec3f(1.0, 1.0, 1.0));
    return mix(mix(mix(n000, n100, u.x), mix(n010, n110, u.x), u.y),
               mix(mix(n001, n101, u.x), mix(n011, n111, u.x), u.y), u.z);
}

// Rotates each octave so the lattices don't line up.
const kOctaveRotation = mat3x3f(0.00, 0.80, 0.60, -0.80, 0.36, -0.48, -0.60, -0.48, 0.64);

fn fbm(position: vec3f, octaves: i32) -> f32
{
    var p = position;
    var amplitude = 0.5;
    var sum = 0.0;
    for (var i = 0; i < octaves; i++)
    {
        sum += amplitude * gradientNoise(p);
        p = kOctaveRotation * p * 2.03;
        amplitude *= 0.5;
    }
    return sum;
}

// Anisotropic spherical Gaussians: max(v.z, 0) * exp(-lambda * v.x^2 - mu * v.y^2), in each lobe's frame.
fn lobes(v: vec3f) -> vec3f
{
    var sum = vec3f(0.0);
    for (var i = 0u; i < uBake.counts.x; i++)
    {
        let z = uBake.lobeZ[i];
        let x = uBake.lobeX[i];
        let c = max(dot(v, z.xyz), 0.0);
        let a = dot(v, x.xyz);
        let b = dot(v, uBake.lobeY[i].xyz);
        sum += uBake.lobeColor[i].rgb * c * exp(-z.w * a * a - x.w * b * b);
    }
    return sum;
}

fn nebula(d: vec3f) -> vec3f
{
    let warpStrength = uBake.params.x;
    let noiseScale = uBake.params.y;
    let dust = uBake.params.z;
    let floorGlow = uBake.params.w;
    let seed = uBake.seed.xyz;
    let stage = uBake.counts.z;

    var floorColor = mix(uBake.floorColorB.rgb, uBake.floorColorA.rgb, 0.5 + 0.5 * d.y) * floorGlow;

    // Two levels of domain warping: q warps the lookup of r, and r warps the direction.
    let p = d * noiseScale + seed;
    var r = vec3f(0.0);
    var dw = d;
    if (stage >= kStageWarp)
    {
        let q = vec3f(fbm(p, 5), fbm(p + vec3f(5.2, 1.3, 2.8), 5), fbm(p + vec3f(1.7, 9.2, 3.4), 5));
        let p2 = p + 4.0 * q;
        r = vec3f(fbm(p2 + vec3f(8.3, 2.8, 4.1), 5), fbm(p2 + vec3f(2.1, 6.4, 7.7), 5), fbm(p2 + vec3f(3.9, 1.2, 5.5), 5));
        dw = normalize(d + warpStrength * r);
    }

    var color = lobes(dw);

    if (stage >= kStageDust)
    {
        for (var i = 0u; i < uBake.counts.y; i++)
        {
            let core = uBake.coreDirection[i];
            let c = max(dot(d, core.xyz), 0.0);
            let cw = max(dot(dw, core.xyz), 0.0);
            color += uBake.coreColor[i].rgb * (exp(-core.w * (1.0 - c)) + 0.08 * exp(-core.w * 0.04 * (1.0 - cw)));
        }

        let lane = smoothstep(0.02, 0.32, fbm(d * noiseScale * 1.7 + 0.8 * r + seed + 17.0, 6));
        color *= exp(-dust * lane * uBake.absorb.rgb);
        floorColor *= mix(1.0, 0.55, clamp(lane * dust * 0.6, 0.0, 1.0));
    }

    return color + floorColor;
}

// The direction through a texel of a cubemap face, with st in [0, 1] and t growing down the face.
fn faceDirection(face: u32, st: vec2f) -> vec3f
{
    let sc = st * 2.0 - 1.0;
    switch face
    {
        case 0u: { return vec3f(1.0, -sc.y, -sc.x); }
        case 1u: { return vec3f(-1.0, -sc.y, sc.x); }
        case 2u: { return vec3f(sc.x, 1.0, sc.y); }
        case 3u: { return vec3f(sc.x, -1.0, -sc.y); }
        case 4u: { return vec3f(sc.x, -sc.y, 1.0); }
        default: { return vec3f(-sc.x, -sc.y, -1.0); }
    }
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f
{
    let st = in.position.xy / f32(uBake.counts.w);
    let direction = normalize(faceDirection(in.face, st));
    let d = (uBake.orientation * vec4f(direction, 0.0)).xyz;
    return vec4f(nebula(d), 1.0);
}
