#version 330 core
uniform vec3 point_color;
out vec4 color;
void main() {
    color = vec4(point_color, 1.f);
}