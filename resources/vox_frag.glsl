#version 450 core 
layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D shadowMapTex;
layout(binding = 2,rgba16f) uniform image3D vox_output;				//voxel image output

in vec3 fragPos;
in vec2 fragTex;
in vec3 fragNor;
in vec4 fragLightSpacePos;

uniform vec3 campos;
uniform vec3 lightpos;
uniform vec3 lightdir;

out vec4 color;

ivec3 voxel_transform(vec3 pos)
	{
	//sponza is in the frame of [-10,10], and we have to map that to [0,255] in x, y, z
	pos += vec3(10,10,10);	//[0,20]
	pos /= 20.;				//[0,1]
	pos*=255;				//[0,255]
	ivec3 ipos = ivec3(pos);
	return ipos;
	}

// Evaluates how shadowed a point is using PCF with 5 samples
// Credit: Sam Freed - https://github.com/sfreed141/vct/blob/master/shaders/phong.frag
float calcShadowFactor(vec4 lightSpacePosition) 
{
    vec3 shifted = (lightSpacePosition.xyz / lightSpacePosition.w + 1.0) * 0.5;

    float shadowFactor = 0;
    float bias = 0.01;
    float fragDepth = shifted.z - bias;

    if (fragDepth > 1.0) {
        return 0.0;
    }

    const int numSamples = 5;
    const ivec2 offsets[numSamples] = ivec2[](
        ivec2(0, 0), ivec2(1, 0), ivec2(0, 1), ivec2(-1, 0), ivec2(0, -1)
    );

    for (int i = 0; i < numSamples; i++) {
        if (fragDepth > textureOffset(shadowMapTex, shifted.xy, offsets[i]).r) {
            shadowFactor += 1;
        }
    }
    shadowFactor /= numSamples;

    return shadowFactor;
}

void main()
{
	vec3 texturecolor = texture(tex, fragTex).rgb;
	float shadowFactor = 1.0 - calcShadowFactor(fragLightSpacePos);
	float ambienceshadowFactor = 0.1 + shadowFactor*0.9;
	
	color.rgb = shadowFactor * texturecolor;
	//color.rgb = vec3(shadowFactor);
	color.a = 1.0f;


	ivec3 voxelpos = voxel_transform(fragPos);
	imageStore(vox_output, voxelpos, color);

}
