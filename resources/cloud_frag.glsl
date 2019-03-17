#version 430 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;

in vec3 fragPos;
in vec2 fragTex;
in vec3 fragNor;
in vec4 fragViewPos;
in vec4 worldPos;

uniform vec2 cloud_offset;
uniform vec3 light_pos;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D tex2;

 void main()
{
	vec4 texturecolor = texture(tex, fragTex +cloud_offset);
	pos_out = worldPos;
	color = texturecolor;

/*
	vec2 texCoord = fragTex+cloud_offset;
  // Calculate vector from pixel to light source in screen space.
  vec2 deltaTexCoord = (texCoord - light_pos.xy);
  
   float Density = 0.5;
  
  // Divide by number of samples and scale by control factor.
  deltaTexCoord *= 1.0f / 3 * Density;
  
  // Store initial sample.
   vec4 texturecolor = texture(tex, fragTex + cloud_offset);

  // Set up illumination decay factor.
   float illuminationDecay = 1.0f;
   
  // Evaluate summation from Equation 3 NUM_SAMPLES iterations.
   for (int i = 0; i < 3; i++)
  {
    // Step sample location along ray.
    texCoord -= deltaTexCoord;

	
    // Retrieve sample at new location.
   vec4 texturecolor2 = texture(tex,texCoord);
   
    // Apply sample attenuation scale/decay factors.
    texturecolor2 *= illuminationDecay * 0.1;

    // Accumulate combined color.
    texturecolor += texturecolor2;

    // Update exponential decay factor.
    illuminationDecay *= (0.2);
	
  }
  
  // Output final color with a further scale control factor.
   color = ( texturecolor * 0.75f);
   */
}
