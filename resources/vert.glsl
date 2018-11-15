#version  450 core
layout(location = 0) in vec4 vertPos;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec2 fragTex;

void main()
{
gl_Position = P * V * M * vertPos;
fragTex = vertTex;
}
