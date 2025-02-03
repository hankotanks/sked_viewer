#version 330 core
in vec2 f_uv;
out vec4 FragColor;
uniform sampler2D sampler;
void main() {
    FragColor = texture(sampler, f_uv);
}