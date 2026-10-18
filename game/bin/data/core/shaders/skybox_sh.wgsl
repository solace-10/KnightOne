// Projects the baked sky onto order 2 spherical harmonics, in the basis order of AmbientSH. Each
// workgroup sums one tile of one face into a partial, which SkyboxBaker adds up once it has read
// them back; the w of the first coefficient carries the solid angle the partial covers.

@group(0) @binding(0) var skyTexture: texture_2d_array<f32>;
@group(0) @binding(1) var<storage, read_write> partials: array<array<vec4f, 9>>;

const kTilesPerSide: u32 = 8u; // Mirrored in SkyboxBaker::kSHTilesPerSide.
const kThreadsPerSide: u32 = 8u;
const kThreads: u32 = kThreadsPerSide * kThreadsPerSide;

var<workgroup> sums: array<array<vec4f, 9>, kThreads>;

// As in skybox_bake.wgsl: the direction through a texel of a cubemap face, with st in [0, 1].
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

@compute @workgroup_size(kThreadsPerSide, kThreadsPerSide)
fn projectMain(@builtin(workgroup_id) group: vec3u, @builtin(local_invocation_id) local: vec3u, @builtin(local_invocation_index) index: u32)
{
    let size = textureDimensions(skyTexture).x;
    let face = group.z;
    let tileSize = (size + kTilesPerSide - 1u) / kTilesPerSide;
    let origin = group.xy * tileSize;
    let texelSize = 2.0 / f32(size);

    var sh: array<vec4f, 9>;
    for (var y = local.y; y < tileSize; y += kThreadsPerSide)
    {
        for (var x = local.x; x < tileSize; x += kThreadsPerSide)
        {
            let texel = origin + vec2u(x, y);
            if (texel.x >= size || texel.y >= size)
            {
                continue;
            }

            // The solid angle a texel subtends shrinks towards the face's corners.
            let st = (vec2f(texel) + 0.5) / f32(size);
            let sc = st * 2.0 - 1.0;
            let solidAngle = texelSize * texelSize / pow(1.0 + dot(sc, sc), 1.5);

            let d = normalize(faceDirection(face, st));
            let l = textureLoad(skyTexture, texel, face, 0).rgb * solidAngle;
            sh[0] += vec4f(l * 0.282095, solidAngle);
            sh[1] += vec4f(l * (0.488603 * d.y), 0.0);
            sh[2] += vec4f(l * (0.488603 * d.z), 0.0);
            sh[3] += vec4f(l * (0.488603 * d.x), 0.0);
            sh[4] += vec4f(l * (1.092548 * d.x * d.y), 0.0);
            sh[5] += vec4f(l * (1.092548 * d.y * d.z), 0.0);
            sh[6] += vec4f(l * (0.315392 * (3.0 * d.z * d.z - 1.0)), 0.0);
            sh[7] += vec4f(l * (1.092548 * d.x * d.z), 0.0);
            sh[8] += vec4f(l * (0.546274 * (d.x * d.x - d.y * d.y)), 0.0);
        }
    }

    sums[index] = sh;
    workgroupBarrier();

    for (var stride = kThreads / 2u; stride > 0u; stride >>= 1u)
    {
        if (index < stride)
        {
            for (var i = 0u; i < 9u; i++)
            {
                sums[index][i] += sums[index + stride][i];
            }
        }
        workgroupBarrier();
    }

    if (index == 0u)
    {
        partials[(face * kTilesPerSide + group.y) * kTilesPerSide + group.x] = sums[0];
    }
}
