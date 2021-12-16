#version 330 core
layout (location = 0) in vec3 Pos;
layout (location = 2) in vec2 TexCoords;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
} vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
    vs_out.FragPos = vec3(model * vec4(Pos, 1.0));
    vs_out.TexCoords = TexCoords;
    gl_Position = projection * view * model * vec4(Pos, 1.0);
}
