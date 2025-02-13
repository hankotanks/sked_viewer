#version 330 core
layout(location = 0) in vec2 v_lam_alf;
out vec3 f_lam_phi;
uniform mat4 proj;
uniform mat4 view;
uniform float globe_radius;
uniform float gst;
void main() {
    float lam = radians(v_lam_alf.x);
    float phi = radians(gst - v_lam_alf.y);
    float x = sin(phi) * cos(lam) * globe_radius;
    float y = cos(phi) * globe_radius;
    float z = sin(phi) * sin(lam) * globe_radius;
    gl_Position = proj * view * vec4(x, y, z, 1.f);
    f_lam_phi = vec3(sin(lam), cos(lam), phi);
}