//#version  430 core
//layout(location = 0) in vec3 vertPos;
//layout(location = 1) in vec3 vertNor;
//layout(location = 2) in vec2 vertTex;
//
//uniform mat4 M;
//uniform float t;
//uniform vec2 resolution;
//
//out vec3 fragNor;
//out vec2 fragTex;
//out vec3 fragPos;
//out vec4 fragViewPos;
//out vec4 worldPos;
//
//void main()
//{
//	fragPos= (M * vec4(vertPos, 1.0)).xyz;
//	fragViewPos= M * vec4(vertPos, 1.0);
//	gl_Position = M * vec4(vertPos, 1.0);
//	worldPos = M * vec4(vertPos, 1.0);
//	fragNor = (M * vec4(vertNor, 0.0)).xyz;
//	fragTex = vertTex;
//
////	if (fragNor.y > 0)
////	{
////		gl_Position.y += (sin(gl_Position.x * t) + cos(gl_Position.z * t)) * .1;
////	}
//}
//

#version  430 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;

uniform mat4 M;
uniform float t;
uniform vec2 resolution;

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

//	float freq = 3.0;
//	float amp = 0.1;
//	float angle = (t + gl_Position.x) * freq;
//	gl_Position.y += (sin(angle)) * amp;
//	worldPos = gl_Position;
//	vec3 newNormals = vec3(-amp * freq * cos(angle), 0.0, 1.0);
//	newNormals = vec3(-amp, freq, cos(angle));
//	newNormals = normalize(newNormals);
//	fragNor = (M * vec4(newNormals, 0.0)).xyz;

	fragNor = (M * vec4(vertNor, 0.0)).xyz;
	fragTex = vertTex;



//	worldPos.z += (sin(gl_Position.x * t) + cos(gl_Position.y * t));
	

//	worldPos.y += (sin(worldPos.x) + cos(worldPos.z)) * .1;
//	fragNor += (sin(fragNor.x) + cos(fragNor.z)) * .1;

//	if (fragNor.y > 0)
//	{
//		
//	}
}
