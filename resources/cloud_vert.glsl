#version  450 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 M;
uniform vec2 InstancePos;

out vec3 fragNor;
out vec2 fragTex;
out vec3 fragPos;
out vec4 fragViewPos;
out vec4 worldPos;

void main()
{
	vec4 pos = vec4(vertPos.x+InstancePos.x, vertPos.y+InstancePos.y, 0, 0);
	worldPos = M * pos;
	fragPos= (M * pos).xyz;
	fragViewPos= M * pos;
	//worldPos = M * vec4(vertPos, 1.0);
	//fragPos= (M * vec4(vertPos, 1.0)).xyz;
	//fragViewPos= M * vec4(vertPos, 1.0);
	gl_Position = vec4(worldPos.xy,0, 1);
	fragNor = (M * vec4(vertNor, 0.0)).xyz;
	fragTex = vertTex;
}
