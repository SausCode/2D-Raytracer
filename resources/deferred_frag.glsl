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

void main()
{
	vec3 texturecolor = texture(col_tex, fragTex).rgb;
	vec3 normals = texture(norm_tex, fragTex).rgb;
	vec3 world_pos = texture(pos_tex, fragTex).rgb;
	

//	//diffuse light
//	vec3 lp = vec3(100,100,100);
//	vec3 ld = normalize(lp - world_pos);
//	float light = dot(ld,normals);	
//	light = clamp(light,0,1);
//
//	//specular light
//	vec3 camvec = normalize(campos - world_pos);
//	vec3 h = normalize(camvec+ld);
//	float spec = pow(dot(h,normals),5);
//	spec = clamp(spec,0,1)*0.3;
//	
//	color.rgb = texturecolor *light + vec3(1,1,1)*spec;
//	color.a=1;

	vec3 fragpos = world_pos;
	vec3 lightpos = vec3(1,1,-3);
	// Light Direction
	vec3 ld = normalize(lightpos-fragpos);
	// Angle between fragment and light
	float a = dot(ld.xy, vec2(1,0));
	// Distance of light to fragment
	float dist = length(lightpos-fragpos);
	//color.rgb = vec3(a);
	color.rgb = texturecolor;
	color.a=1;

	if (pass == 1){
		// First Pass
		vec3 fragpos = world_pos;
		vec3 lightpos = vec3(1,1,-3);
		// Light Direction
		vec3 ld = normalize(lightpos-fragpos);
		// Angle between fragment and light
		float a = dot(ld.xy, vec2(1,0));
		// Distance of light to fragment
		float dist = length(lightpos-fragpos);

		// Differentiate between negative and positive angles
		if (ld.y < 0){
			a = 2-a;
		}

		// Map result from -1 -> 3 to 0 -> 1023
		a += 1;
		a *= 1023./4.;

		color.rgb = vec3(a);

		// Convert float to int
		int idist = int(dist*1e8);
		int bufferindex = int(a);
		atomicMin(angle_list[bufferindex].y, idist);
	}

	else{
		// Second Pass
		vec3 fragpos = world_pos;
		vec3 lightpos = vec3(1,1,-3);
		// Light Direction
		vec3 ld = normalize(lightpos-fragpos);
		// Angle between fragment and light
		float a = dot(ld.xy, vec2(1,0));
		// Distance of light to fragment
		float dist = length(lightpos-fragpos);

		// Differentiate between negative and positive angles
		if (ld.y < 0){
			a = 2-a;
		}

		// Map result from -1 -> 3 to 0 -> 1023
		a += 1;
		a *= 1023./4.;
		int idist = int(dist*1e8);
		int bufferindex = int(a);

		int comp_dist = angle_list[bufferindex].y;

		if (comp_dist > dist/1e8){
			//color.rgb = texturecolor;
		}

		else{
			//color.rgb = vec3(0);
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
