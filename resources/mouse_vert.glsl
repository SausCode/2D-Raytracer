#version  430 core
in vec4 vertPos;
in vec3 vertNor;
in vec2 vertTex;
uniform mat4 P;
uniform mat4 M;
uniform mat4 V;
out vec3 fragNor;
out vec2 fragTex;
out vec3 pos;

void main()
{
    pos=vertPos.xyz;
    gl_Position = P * V * M * vertPos;
    fragNor = (M * vec4(vertNor, 0.0)).xyz;
    fragTex = vertTex;
}
