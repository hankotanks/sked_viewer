#version 330 core
layout(location = 0) in vec3 v_pos;
out vec3 f_pos;
uniform mat4 proj;
uniform mat4 view;
void main() {
    gl_Position = proj * view * vec4(v_pos, 1.0);
    f_pos = v_pos;
}