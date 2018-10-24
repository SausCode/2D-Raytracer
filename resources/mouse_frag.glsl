#version 430 core 
in vec3 fragNor;
out vec4 color;
in vec3 pos;
in vec2 fragTex;


void main()
{
	vec3 normal = normalize(fragNor);

	vec3 lp = vec3(100,100,100);
	vec3 ld = normalize(lp - pos);
	float light = length(ld);
	color.rgb = vec3(1,1,0);

	color.a=1;
	 
}
