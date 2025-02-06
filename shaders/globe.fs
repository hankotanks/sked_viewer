#version 330 core
in vec3 f_pos;
out vec4 color;
uniform sampler2D globe_sampler;
void main() {
    vec3 f_pos_normal = normalize(f_pos);
    float lam = atan(f_pos_normal.x, f_pos_normal.z);
    float phi = acos(f_pos_normal.y);
    float u = (lam + radians(180.f)) / radians(360.f);
    float v = 1.f - phi / radians(180.f);
    color = texture(globe_sampler, vec2(u, v));
}