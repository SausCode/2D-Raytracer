#version 430
out vec4 color;
in vec2 fragTex;

layout(location = 0) uniform sampler2D col_tex;
layout(location = 1) uniform sampler2D pos_tex;
layout(location = 2) uniform sampler2D norm_tex;

vec3 voxel_transform(vec3 pos)
{
	//sponza is in the frame of [-1,1], and we have to map that to [0,255] in x, y, z
	vec3 texpos = pos + vec3(1, 1, 1);	//[0,2]
	texpos /= 2;
	return texpos;
}

vec4 sampling(vec3 texposition, float mipmap)
{
	uint imip = uint(mipmap);
	float linint = mipmap - float(imip);
	vec4 colA = texture(col_tex, fragTex, imip);
	vec4 colB = texture(col_tex, fragTex, imip + 1);
	return mix(colA, colB, linint);
}

vec3 cone_tracing(vec3 conedirection, vec3 pixelpos)
{
	conedirection = normalize(conedirection);
	float voxelSize = 2. / 256.;				//[-1,1] / resolution	
												//pixelpos += conedirection*voxelSize;	//to get some distance to the pixel against self-inducing
	vec4 trace = vec4(0);
	float distanceFromConeOrigin = voxelSize * 2;
	float coneHalfAngle = 0.471239; //27 degree
	for (int i = 0; i < 10; i++)
	{
		float coneDiameter = 2 * tan(coneHalfAngle) * distanceFromConeOrigin;
		float mip = log2(coneDiameter / voxelSize);
		pixelpos = pixelpos + conedirection * distanceFromConeOrigin;
		vec3 texpos = voxel_transform(pixelpos);
		trace += sampling(texpos, mip);
		if (trace.a > 0.7) {
			break;
		}
		distanceFromConeOrigin += voxelSize;
	}
	return trace.rgb;
}

void main()
{
	color.a = 1;
	vec3 texturecolor = texture(col_tex, fragTex).rgb;
	vec3 normals = texture(norm_tex, fragTex).rgb;
	vec3 world_pos = texture(pos_tex, fragTex).rgb;
	vec3 voxelcolor = cone_tracing(normals, world_pos);
	float magn = length(voxelcolor);
	color.rgb = texturecolor + voxelcolor;

	//color.rgb = world_pos;
}