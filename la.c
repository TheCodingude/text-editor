typedef struct {
    float x, y, z, w;
} Vec4f;

// Create a new vector
Vec4f vec4f(float x, float y, float z, float w) {
    Vec4f v = {x, y, z, w};
    return v;
}

// Add two vectors
Vec4f Vec4f_add(Vec4f a, Vec4f b) {
    return vec4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

// Subtract two vectors
Vec4f Vec4f_sub(Vec4f a, Vec4f b) {
    return vec4f(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

// Scale a vector
Vec4f Vec4f_scale(Vec4f v, float s) {
    return vec4f(v.x * s, v.y * s, v.z * s, v.w * s);
}

// Dot product
float Vec4f_dot(Vec4f a, Vec4f b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}