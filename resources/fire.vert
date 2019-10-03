#version  430 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
uniform mat4 M;

out vec3 fragNor;
out vec2 fragTex;
out vec4 pos;

void main()
{
    pos = M * vec4(vertPos, 1.0);
    gl_Position = M * vec4(vertPos, 1.0);
    fragNor = (M * vec4(vertNor, 0.0)).xyz;
    fragTex = vertTex;
}
