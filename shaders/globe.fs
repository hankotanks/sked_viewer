#version 330 core
in vec3 f_pos;
out vec4 color;
uniform sampler2D globe_sampler;
void main() {
    vec3 f_pos_normal = normalize(f_pos);
    float theta = atan(f_pos_normal.z, f_pos_normal.x);
    float phi = acos(f_pos_normal.y);
    float u = 1.f - (theta + radians(180.0)) / radians(360.0);
    float v = 1.f - phi / radians(180.0);
    color = texture(globe_sampler, vec2(u, v));
}