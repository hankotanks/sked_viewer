#version 330 core
layout(location = 0) in vec2 v_lam_phi;
out vec3 f_lam_phi;
uniform mat4 proj;
uniform mat4 view;
uniform float globe_radius;
void main() {
    float lam = radians(v_lam_phi.x);
    float phi = radians(v_lam_phi.y);
    float x = sin(phi) * cos(lam) * globe_radius;
    float y = cos(phi) * globe_radius;
    float z = sin(phi) * sin(lam) * globe_radius;
    gl_Position = proj * view * vec4(x, y, z, 1.f);
    f_lam_phi = vec3(sin(lam), cos(lam), phi);
}