#version 430 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;

in vec3 fragNor;
in vec3 pos;
in vec2 fragTex;


void main()
{
	vec3 normal = normalize(fragNor);

	vec3 lp = vec3(100,100,100);
	vec3 ld = normalize(lp - pos);
	float light = length(ld);
	color.rgb = vec3(1,1,0);
	pos_out.rgb = vec3(1,1,0);
	norm_out.rgb = vec3(1,1,0);

	color.a=1;

	color.rgb = vec3(1);
	 
}
