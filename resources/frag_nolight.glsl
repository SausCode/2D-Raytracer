#version 450 core 
out vec4 color;
in vec2 fragTex;
in vec3 fragNor;
layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D normal_tex;
layout(location = 2) uniform sampler2D pos_tex;
layout(location = 3) uniform sampler2D blur_tex;

uniform vec3 campos;

uniform float bloom;

void main()
{
	vec3 texturecolor = texture(tex, fragTex).rgb;
	vec3 normals = texture(normal_tex, fragTex).rgb;
	vec3 world_pos = texture(pos_tex, fragTex).rgb;
	vec3 blur = texture(blur_tex, fragTex).rgb;

	color.rgb = texturecolor;
	color.rgb = world_pos;
	color.rgb = normals;
	color.a=1;

	//diffuse light
	vec3 lp = vec3(100,100,100);
	vec3 ld = normalize(lp - world_pos);
	float light = dot(ld,normals);	
	light = clamp(light,0,1);

	//specular light
	vec3 camvec = normalize(campos - world_pos);
	vec3 h = normalize(camvec+ld);
	float spec = pow(dot(h,normals),5);
	spec = clamp(spec,0,1)*0.3;
	
	color.rgb = texturecolor *light + vec3(1,1,1)*spec;

	float xp = 1./(640*2);
	float yp = 1./(480*2);

	if (bloom > .5){
		for (int i = -5; i <= 5; i++){
			if (i == 0){
				continue;
			}
			vec3 col = texture(blur_tex, fragTex+vec2(i*xp, 0)).rgb;
			color.rgb += col* 1./abs(i);
		}
		for (int i = -5; i <= 5; i++){
			if (i == 0){
				continue;
			}
			vec3 col = texture(blur_tex, fragTex+vec2(0, i*yp)).rgb;
			color.rgb += col* 1./abs(i);
		}
	}

//	better results with HDR!
}









































//#version 450 core 
//
//layout(location = 0) out vec4 color;
//layout(location = 1) out vec4 viewpos;
//
//in vec3 fragPos;
//in vec2 fragTex;
//in vec3 fragNor;
//in vec4 fragViewPos;
//
//uniform vec3 campos;
//
//layout(location = 0) uniform sampler2D tex;
//layout(location = 1) uniform sampler2D tex2;
//
//
//void main()
//{
//	vec3 normal = normalize(fragNor);
//	vec3 texturecolor = texture(tex, fragTex).rgb;
//	
//	//diffuse light
//	vec3 lp = vec3(100,100,100);
//	vec3 ld = normalize(lp - fragPos);
//	float light = dot(ld,normal);	
//	light = clamp(light,0,1);
//
//	//specular light
//	vec3 camvec = normalize(campos - fragPos);
//	vec3 h = normalize(camvec+ld);
//	float spec = pow(dot(h,normal),5);
//	spec = clamp(spec,0,1)*0.3;
//	
//	color.rgb = texturecolor *light + vec3(1,1,1)*spec;
//	color.rgb = texturecolor;
//	color.a=1;
//	viewpos = fragViewPos;
//	viewpos.z*=-1;
//
//}
//
