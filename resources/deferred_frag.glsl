#version 430
#extension GL_ARB_shader_storage_buffer_object : require

#define ssbo_size 2048
layout (std430, binding=0) volatile buffer shader_data
{
	ivec4 angle_list[ssbo_size];
};

out vec4 color;
in vec2 fragTex;
in vec3 fragNor;
layout(location = 0) uniform sampler2D col_tex;
layout(location = 1) uniform sampler2D pos_tex;
layout(location = 2) uniform sampler2D norm_tex;

// 1 for first, 2 for second
uniform int pass;
uniform vec3 light_pos;

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void main()
{
	color.a = 1;

	vec3 texturecolor = texture(col_tex, fragTex).rgb;
	vec3 normals = texture(norm_tex, fragTex).rgb;
	vec3 world_pos = texture(pos_tex, fragTex).rgb;
	
	vec2 fragpos = world_pos.xy;
	vec3 lightpos = light_pos;
	// Light Direction
	vec2 ld = normalize(fragpos.xy-lightpos.xy);
	// Angle between fragment and light
	float angle = dot(ld.xy, vec2(1,0));
	// Distance of light to fragment
	float dist = length(lightpos.xy-fragpos.xy);
	// Differentiate between negative and positive angles
	if (ld.y < 0){
		angle = 2-angle;
	}

	// Map result from -1 -> 3 to 0 -> ssbo_size
	int distance_converted = int(map(dist, 0, 2.828, 0, 10000));
	float angle_converted = map(angle, -1, 3, 0, 1);
	int bufferindex = int(angle_converted*ssbo_size);
	if (pass == 1){
		// Convert float to int
		atomicMin(angle_list[bufferindex].y, distance_converted);
		memoryBarrier();
	}

	else{
		if (angle_list[bufferindex].y > (distance_converted-500.00))
		{
			float d = abs(angle_list[bufferindex].y - distance_converted)/500.;
			d=pow(1-d,2);
			color.rgb = texturecolor *d;

//			//diffuse light
//			vec3 lp = vec3(100,100,100);
//			vec3 ld = normalize(lp - world_pos);
//			float light = dot(ld,normals);	
//			light = clamp(light,0,1);
//			//specular light
//			vec3 camvec = normalize(campos - world_pos);
//			vec3 h = normalize(camvec+ld);
//			float spec = pow(dot(h,normals),5);
//			spec = clamp(spec,0,1)*0.3;
//			color.rgb = texturecolor *light + vec3(1,1,1)*spec;
		}
		else{
			color.rgb = vec3(0,0,0);
		}
	}
}