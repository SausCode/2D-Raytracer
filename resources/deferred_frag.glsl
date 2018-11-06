#version 430
#extension GL_ARB_shader_storage_buffer_object : require
//layout(local_size_x = 1024, local_size_y = 1) in;

layout (std430, binding=0) volatile buffer shader_data
{
	ivec4 angle_list[1024];
};

out vec4 color;
in vec2 fragTex;
in vec3 fragNor;
layout(location = 0) uniform sampler2D col_tex;
layout(location = 1) uniform sampler2D pos_tex;
layout(location = 2) uniform sampler2D norm_tex;

uniform vec3 campos;
// 1 for first, 2 for second
uniform int pass;
uniform vec3 light_pos;

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void main()
{
	vec3 texturecolor = texture(col_tex, fragTex).rgb;
	vec3 normals = texture(norm_tex, fragTex).rgb;
	vec3 world_pos = texture(pos_tex, fragTex).rgb;

	vec2 fragpos = world_pos.xy;
	vec3 lightpos = light_pos;
	// Light Direction
	vec2 ld = normalize(fragpos.xy-lightpos.xy);
	// Angle between fragment and light
	float a = dot(ld.xy, vec2(1,0));
	// Distance of light to fragment
	float dist = length(lightpos.xy-fragpos.xy);
	// Differentiate between negative and positive angles
	float b=a;
	if (ld.y < 0){
		a = 2-a;
	}
	if (equal(vec3(0,0,0), texturecolor).x){
		color.rgb = vec3(0,0,0);
		return;
	}
	

	// Map result from -1 -> 3 to 0 -> 1023
	a += 1;
	a *= 1023./4.;
	a/=1023;
	//a = map(a, 0, 1023, 0, 1);
	color.rgb = vec3((b+1)/2.);
	//color.rgb = texturecolor;
	//color.rgb = vec3(map(dist, 0, 2.828, 0, 1));
	color.a=1;
	int distance_converted = int(map(dist, 0, 2.828, 0, 3000000));
	float b_converted = (b+1.)/2.;
	color.rgb = vec3(b_converted);
	//return;

	if (pass == 1){
		// Convert float to int
		int bufferindex = int(b*1024);
		atomicMin(angle_list[bufferindex].y, distance_converted);
	}

	else{
		int bufferindex = int(b*1024);
		color.rgb = vec3(map(angle_list[bufferindex].y, 0, 3000000, 0, 1));
		if (angle_list[bufferindex].y > 0){
			color.rgb = vec3(b);
		}
		else{
			color.rgb = vec3(0);
		}
	}

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
