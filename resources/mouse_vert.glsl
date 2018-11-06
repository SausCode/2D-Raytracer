#version  430 core
in vec4 vertPos;
in vec3 vertNor;
in vec2 vertTex;
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
