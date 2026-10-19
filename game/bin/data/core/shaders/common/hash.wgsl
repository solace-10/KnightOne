// Three pseudo-random values in [0, 1) from a 3D position.
fn hash33(p: vec3f) -> vec3f
{
    var p3 = fract(p * vec3f(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yxx) * p3.zyx);
}
