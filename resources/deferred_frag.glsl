#version 430
#extension GL_ARB_shader_storage_buffer_object : require

#define ssbo_size 2048
layout(std430, binding = 0) volatile buffer shader_data
{
	ivec4 angle_list[ssbo_size];
};

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;

in vec2 fragTex;
in vec3 fragNor;
layout(location = 0) uniform sampler2D col_tex;
layout(location = 1) uniform sampler2D pos_tex;
layout(location = 2) uniform sampler2D norm_tex;

// 1 for first, 2 for second
uniform int pass;
uniform vec3 light_pos;
uniform vec3 campos;
uniform int screen_width;
uniform int screen_height;

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

vec2 fragTopAndBottomAngles(vec2 fragpos, vec3 lightpos){
	vec2 lower_left = fragpos;
	vec2 lower_right = vec2(fragpos.x+(1.0f/screen_width), fragpos.y);
	vec2 upper_right = vec2(fragpos.x+(1.0f/screen_width), fragpos.y+(1.0f/screen_width));
	vec2 upper_left = vec2(fragpos.x, fragpos.y+(1.0f/screen_width));

	float min, max;
	vec2 pos[4] = vec2[](lower_left, lower_right, upper_right, upper_left);
	float angles[4];

	for(int i=0; i<4; i++){
		// Light Direction
		vec2 ld = normalize((pos[i]).xy-lightpos.xy);
		// Angle between fragment and light
		float angle = dot(ld.xy, vec2(1,0));
		// Differentiate between negative and positive angles
		if (ld.y < 0){
			angle = 2-angle;
		}

		angles[i] = angle;

	}

	min = angles[0];
	max = angles[0];

	for(int i=1; i<4; i++){
		if(angles[i]<min)
			min = angles[i];
		else if(angles[i]>max)
			max = angles[i];
	}

	return vec2(min, max);
}

void main()
{
	color.a = 1;

	vec3 texturecolor = texture(col_tex, fragTex).rgb;
	vec3 normals = texture(norm_tex, fragTex).rgb;
	vec3 world_pos = texture(pos_tex, fragTex).rgb;
	normals *= -1;
	vec2 fragpos = world_pos.xy;
	vec3 lightpos = light_pos;

	color.rgb = normals;
	//return;
	vec2 angle_range = fragTopAndBottomAngles(fragpos, lightpos);
	float min_angle = angle_range.x;
	float max_angle = angle_range.y;

	float min_angle_converted = map(min_angle, -1, 3, 0, 1);
	int min_bufferindex = int(min_angle_converted*ssbo_size);
	float max_angle_converted = map(max_angle, -1, 3, 0, 1);
	int max_bufferindex = int(max_angle_converted*ssbo_size);

	
	// Distance of light to fragment
	float dist = length(lightpos.xy-fragpos.xy);
	// Map result from -1 -> 3 to 0 -> ssbo_size
	int distance_converted = int(map(dist, 0, 2.828, 0, 10000));
	
	for(int i=min_bufferindex; i<=max_bufferindex; i++){

		if (pass == 1) {
			// Convert float to int
			atomicMin(angle_list[i].y, distance_converted);
			memoryBarrier();
		   
		}

		else {
			if (angle_list[i].y > (distance_converted - 500.00))
			{
				
				float d = abs(angle_list[i].y - distance_converted) / 500.;
				d = pow(1 - d, 2);
				color.rgb = texturecolor * d;
				//diffuse light
				vec3 lp = vec3(lightpos.xy, 0);
				vec3 ld = normalize(lp - world_pos);
				if(normals.r!=0){
					float light = dot(ld, normals);
					light = clamp(light, 0, 1);
					color.rgb = texturecolor * d*light;
				}
				
				if(normals.r==0 && color.r!=0){
					vec2 texCoord = world_pos.xy;
					// Calculate vector from pixel to light source in screen space.
					vec2 deltaTexCoord = normalize(lightpos.xy-texCoord);
  
					float Density = 0.5;
  
					// Divide by number of samples and scale by control factor.
					deltaTexCoord *= 1.0f / 3 * Density;

					// Set up illumination decay factor.
					float illuminationDecay = 1.0f;
   
					// Evaluate summation from Equation 3 NUM_SAMPLES iterations.
					for (int i = 0; i < 3; i++)
					{
						// Step sample location along ray.
						texCoord -= deltaTexCoord;
					
	
						// Retrieve sample at new location.
						vec3 texturecolor2 = texture(col_tex,texCoord).rgb;
   
						// Apply sample attenuation scale/decay factors.
						texturecolor2 *= illuminationDecay*0.1f;
					
						// Accumulate combined color.
						texturecolor += texturecolor2;

						// Update exponential decay factor.
						illuminationDecay *= (0.5);
	
					}
  
					// Output final color with a further scale control factor.
					color.rgb = (texturecolor*0.75f)*d;
				}
			}
			else {
				color.rgb = vec3(0, 0, 0);
			}
		}
		
	}
	//color.rgb = texturecolor;

	norm_out = vec4(normals, 1);
	pos_out = vec4(world_pos, 1);
}