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
