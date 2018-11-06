#version  430 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 M;

out vec3 fragNor;
out vec2 fragTex;
out vec3 fragPos;
out vec4 fragViewPos;
out vec4 worldPos;

void main()
{
	fragPos= (M * vec4(vertPos, 1.0)).xyz;
	fragViewPos= M * vec4(vertPos, 1.0);
	gl_Position = M * vec4(vertPos, 1.0);
	worldPos = M * vec4(vertPos, 1.0);
	fragNor = (M * vec4(vertNor, 0.0)).xyz;
	fragTex = vertTex;
}
