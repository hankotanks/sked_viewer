#version 330 core
layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_uv;
out vec2 f_uv;
uniform mat4 proj;
uniform mat4 view;
void main() {
    gl_Position = proj * view * vec4(v_pos, 1.0);
    f_uv = v_uv;
}