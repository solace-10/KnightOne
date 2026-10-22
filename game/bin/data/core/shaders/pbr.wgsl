// PBR Shader - Metallic-Roughness Workflow (GLTF 2.0 Specification)
// Cook-Torrance BRDF with GGX distribution

const PI: f32 = 3.14159265359;

struct VertexInput
{
    @builtin(instance_index) instanceIdx: u32,
    @location(0) position: vec3f,
    @location(1) normal: vec3f,
    @location(2) uv: vec2f,
    @location(4) tangent: vec4f
};

struct VertexOutput
{
    @builtin(position) position: vec4f,
    @location(0) worldPosition: vec3f,
    @location(1) worldNormal: vec3f,
    @location(2) worldTangent: vec4f,
    @location(3) uv: vec2f
};

struct LocalUniforms
{
    modelMatrix: mat4x4<f32>
};

struct InstanceUniforms
{
    transform: array<mat4x4<f32>>
};

#include "/shaders/pandora/globals.wgsl"
#include "/shaders/pandora/shadows.wgsl"
@group(1) @binding(0) var<uniform> uLocalUniforms: LocalUniforms;
@group(2) @binding(0) var<storage, read> uInstanceUniforms: InstanceUniforms;
@group(3) @binding(0) var defaultSampler: sampler;
@group(3) @binding(1) var baseColorTexture: texture_2d<f32>;
@group(3) @binding(2) var metallicRoughnessTexture: texture_2d<f32>;
@group(3) @binding(3) var normalTexture: texture_2d<f32>;
@group(3) @binding(4) var occlusionTexture: texture_2d<f32>;
@group(3) @binding(5) var emissiveTexture: texture_2d<f32>;

@vertex fn vertexMain(in: VertexInput) -> VertexOutput
{
    var out: VertexOutput;
    let modelMatrix = uInstanceUniforms.transform[in.instanceIdx] * uLocalUniforms.modelMatrix;
    out.position = uGlobalUniforms.projectionMatrix * uGlobalUniforms.viewMatrix * modelMatrix * vec4f(in.position, 1.0);
    out.worldPosition = (modelMatrix * vec4f(in.position, 1.0)).xyz;
    out.worldNormal = normalize((modelMatrix * vec4f(in.normal, 0.0)).xyz);
    out.worldTangent = vec4f(normalize((modelMatrix * vec4f(in.tangent.xyz, 0.0)).xyz), in.tangent.w);
    out.uv = in.uv;
    return out;
}

// Compute TBN matrix from screen-space derivatives (for normal mapping without tangent attributes)
fn computeTBNScreenSpace(worldPos: vec3f, worldNormal: vec3f, uv: vec2f) -> mat3x3<f32>
{
    let dPdx = dpdx(worldPos);
    let dPdy = dpdy(worldPos);
    let dUVdx = dpdx(uv);
    let dUVdy = dpdy(uv);

    // Solve the linear system to get tangent and bitangent
    let r = 1.0 / (dUVdx.x * dUVdy.y - dUVdx.y * dUVdy.x);
    let tangent = normalize((dPdx * dUVdy.y - dPdy * dUVdx.y) * r);
    let bitangent = normalize((dPdy * dUVdx.x - dPdx * dUVdy.x) * r);

    // Re-orthogonalize tangent with respect to normal using Gram-Schmidt
    let N = normalize(worldNormal);
    let T = normalize(tangent - N * dot(N, tangent));
    let B = cross(N, T);

    return mat3x3<f32>(T, B, N);
}

fn computeTBN(worldNormal: vec3f, worldTangent: vec4f) -> mat3x3<f32>
{
    let N = normalize(worldNormal);
    let T = normalize(worldTangent).xyz;
    let B = cross(N, T) * worldTangent.w;
    return mat3x3<f32>(T, B, N);
}

// Fresnel-Schlick approximation
fn fresnelSchlick(cosTheta: f32, F0: vec3f) -> vec3f
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// GGX/Trowbridge-Reitz Normal Distribution Function
fn distributionGGX(N: vec3f, H: vec3f, roughness: f32) -> f32
{
    let a = roughness * roughness;
    let a2 = a * a;
    let NdotH = max(dot(N, H), 0.0);
    let NdotH2 = NdotH * NdotH;

    let num = a2;
    let denom = (NdotH2 * (a2 - 1.0) + 1.0);
    let denomSq = PI * denom * denom;

    return num / denomSq;
}

// Schlick-GGX Geometry Function (single direction)
fn geometrySchlickGGX(NdotV: f32, roughness: f32) -> f32
{
    let r = roughness + 1.0;
    let k = (r * r) / 8.0;

    let num = NdotV;
    let denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

// Smith's Geometry Function (both directions)
fn geometrySmith(N: vec3f, V: vec3f, L: vec3f, roughness: f32) -> f32
{
    let NdotV = max(dot(N, V), 0.0);
    let NdotL = max(dot(N, L), 0.0);
    let ggx1 = geometrySchlickGGX(NdotV, roughness);
    let ggx2 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// The ambient light reflected by a white Lambertian surface facing n. See AmbientSH in
// lighting_system.hpp for the basis order. Clamped, as bright spots on the sky make the SH ring
// below zero on the far side.
fn ambientSH(n: vec3f) -> vec3f
{
    let sh = uGlobalUniforms.ambientSH;
    let c = 0.282095 * sh[0].rgb
        + 0.488603 * (n.y * sh[1].rgb + n.z * sh[2].rgb + n.x * sh[3].rgb)
        + 1.092548 * (n.x * n.y * sh[4].rgb + n.y * n.z * sh[5].rgb + n.x * n.z * sh[7].rgb)
        + 0.315392 * (3.0 * n.z * n.z - 1.0) * sh[6].rgb
        + 0.546274 * (n.x * n.x - n.y * n.y) * sh[8].rgb;
    return max(c, vec3f(0.0));
}

// Karis' analytic fit to the split-sum environment BRDF, from "Physically Based Shading on Mobile".
fn environmentBRDF(F0: vec3f, roughness: f32, NdotV: f32) -> vec3f
{
    let c0 = vec4f(-1.0, -0.0275, -0.572, 0.022);
    let c1 = vec4f(1.0, 0.0425, 1.04, -0.04);
    let r = roughness * c0 + c1;
    let a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
    let AB = vec2f(-1.04, 1.04) * a004 + r.zw;
    return F0 * AB.x + AB.y;
}

@fragment fn fragmentMain(in: VertexOutput) -> @location(0) vec4f
{
    // All textures will already be in linear space: base color and emissive
    // are converted into linear when they're loaded.
    let baseColor = textureSample(baseColorTexture, defaultSampler, in.uv).rgb;

    let metallicRoughness = textureSample(metallicRoughnessTexture, defaultSampler, in.uv);
    let metallic = metallicRoughness.b; // GLTF: metallic in blue channel
    let roughness = metallicRoughness.g; // GLTF: roughness in green channel

    let normalMapSample = textureSample(normalTexture, defaultSampler, in.uv).rgb;
    let tangentNormal = normalMapSample * 2.0 - 1.0; // Unpack from [0,1] to [-1,1]

    var ao = textureSample(occlusionTexture, defaultSampler, in.uv).r;
    let emissive = textureSample(emissiveTexture, defaultSampler, in.uv).rgb;

    // Compute TBN matrix and transform normal to world space
    let TBN = computeTBN(in.worldNormal, in.worldTangent);
    let N = normalize(TBN * tangentNormal);

    // View and light directions
    let V = normalize(uGlobalUniforms.cameraPosition.xyz - in.worldPosition);
    let L = normalize(uGlobalUniforms.directionalLightDirection.xyz);
    let H = normalize(V + L);

    // Calculate reflectance at normal incidence (F0)
    // Dielectrics use 0.04, metals use the base color
    let F0 = mix(vec3f(0.04), baseColor, metallic);

    // Cook-Torrance BRDF
    let NDF = distributionGGX(N, H, roughness);
    let G = geometrySmith(N, V, L, roughness);
    let F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    // Specular contribution
    let numerator = NDF * G * F;
    let NdotV = max(dot(N, V), 0.0);
    let NdotL = max(dot(N, L), 0.0);
    let denominator = 4.0 * NdotV * NdotL + 0.0001; // Prevent divide by zero
    let specular = numerator / denominator;

    // Energy conservation: diffuse and specular cannot exceed 1.0
    let kS = F; // Specular contribution (Fresnel term)
    let kD = (vec3f(1.0) - kS) * (1.0 - metallic); // Diffuse only for non-metals

    // Lambertian diffuse
    let diffuse = kD * baseColor / PI;

    // Outgoing radiance from directional light
    let lightColor = uGlobalUniforms.directionalLightColor.rgb;
    let shadow = sampleShadow(in.worldPosition, normalize(in.worldNormal));
    let Lo = (diffuse + specular) * lightColor * NdotL * shadow;

    // Ambient lighting with occlusion. The specular half reads the SH in the reflection direction,
    // which is only as sharp as the SH are, so glossy surfaces reflect a very blurred sky.
    let ambientSpecular = environmentBRDF(F0, roughness, NdotV);
    let ambientDiffuse = (vec3f(1.0) - ambientSpecular) * (1.0 - metallic) * baseColor;
    let R = reflect(-V, N);
    let ambient = (ambientDiffuse * ambientSH(N) + ambientSpecular * ambientSH(R)) * ao;

    // Final color: ambient + direct lighting + emissive
    let color = ambient + Lo + emissive;  
    return vec4f(color, 1.0);
}
