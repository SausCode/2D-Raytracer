#version  430 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
uniform mat4 M;
out vec3 fragNor;
out vec2 fragTex;
out vec3 pos;

void main()
{
    pos = (M * vertPos).xyz;
    gl_Position = M * vertPos;
    fragNor = (M * vec4(vertNor, 0.0)).xyz;
    fragTex = vertTex;
}
