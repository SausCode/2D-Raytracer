//#version  430 core
//in vec4 vertPos;
//in vec3 vertNor;
//in vec2 vertTex;
//in vec3 vertTan;
//in vec3 vertBinorm;
//uniform mat4 P;
//uniform mat4 M;
//uniform mat4 V;
//out vec3 fragNor;
//out vec2 fragTex;
//out vec3 fragTan;
//out vec3 fragBinorm;
//out vec3 pos;
//out vec4 worldPos;
//
//void main()
//{
//    pos=vertPos.xyz;
//    gl_Position = P * V * M * vertPos;
//	worldPos = M * vec4(vertPos.xyz,1);
//    fragNor = (M * vec4(vertNor, 0.0)).xyz;
//    fragTan = (M * vec4(vertTan, 0.0)).xyz;
//    fragBinorm = (M * vec4(vertBinorm, 0.0)).xyz;
//    fragTex = vertTex;
//}


#version  430 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;

out vec3 fragNor;
out vec2 fragTex;
out vec3 fragPos;
out vec4 fragViewPos;
out vec4 worldPos;

void main()
{
	fragPos= (M * vec4(vertPos, 1.0)).xyz;
	fragViewPos=V * M * vec4(vertPos, 1.0);
	gl_Position = P * V * M * vec4(vertPos, 1.0);
	worldPos = V * M * vec4(vertPos, 1.0);
	fragNor = (M * vec4(vertNor, 0.0)).xyz;
	fragTex = vertTex;
}
