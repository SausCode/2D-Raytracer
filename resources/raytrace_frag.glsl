#version 430
out vec4 color;
in vec2 fragTex;

layout(location = 0) uniform sampler2D col_tex;
layout(location = 1) uniform sampler2D pos_tex;
layout(location = 2) uniform sampler2D norm_tex;
layout(location = 3) uniform sampler2D no_light_tex;

uniform float dovoxel;
uniform vec3 mouse_pos;

vec2 voxel_transform(vec2 pos)
{
	//sponza is in the frame of [-1,1], and we have to map that to [0,255] in x, y, z
	vec2 texpos = pos + vec2(1, 1);	//[0,2]
	texpos /= 2;
	return texpos;
}

vec4 sampling(vec2 conedirection, vec2 texposition, float mipmap)
{




	uint imip = uint(mipmap);
	float linint = mipmap - float(imip);
	//vec4 colA = texture(no_light_tex, fragTex, imip);
	//vec4 colB = texture(no_light_tex, fragTex, imip + 1);
	//	imip=0;
	vec4 colA = texture(col_tex, texposition.xy, imip);
	vec4 colB = texture(col_tex, texposition.xy, imip + 1);

	vec2 norm = -texture(norm_tex, texposition.xy, 1).xy;

	float d = dot(normalize(norm), normalize(conedirection));
	d = clamp(d, 0, 1);


	vec4 col = mix(colA, colB, linint)*d;


	return col;
}

vec3 cone_tracing(vec2 conedirection, vec2 pixelpos, float angle)
{
	conedirection = normalize(conedirection);
	float voxelSize = 2. / 256.;				//[-1,1] / resolution	
	voxelSize = 0.01000003103;
	//pixelpos += conedirection*voxelSize;	//to get some distance to the pixel against self-inducing
	vec4 trace = vec4(0);
	float distanceFromConeOrigin = voxelSize * 2;
	float coneHalfAngle = angle;
	for (int i = 0; i < 15; i++)
	{
		float coneDiameter = 2 * tan(coneHalfAngle) * distanceFromConeOrigin;
		float mip = log2(coneDiameter / voxelSize);
		pixelpos = pixelpos + conedirection * distanceFromConeOrigin;
		vec2 texpos = voxel_transform(pixelpos);
		trace += sampling(conedirection, texpos, mip);
		//if (trace.a > 0.7) {
		//break;
		//}
		distanceFromConeOrigin += voxelSize;
	}
	return trace.rgb;
}

void main()
{
	color.a = 1;
	float coneHalfAngle = 0.051571239;
	vec3 texturecolor = texture(col_tex, fragTex).rgb;
	vec3 normals = texture(norm_tex, fragTex).rgb;
	vec3 world_pos = texture(pos_tex, fragTex).rgb;
	vec3 voxelcolor = cone_tracing(normals.xy, world_pos.xy, coneHalfAngle);
	//voxelcolor += cone_tracing(normals.xy + vec2(.1, .1), world_pos.xy, coneHalfAngle);
	//voxelcolor += cone_tracing(normals.xy + vec2(.1, -.1), world_pos.xy, coneHalfAngle);
	//voxelcolor += cone_tracing(normals.xy + vec2(-.1, .1), world_pos.xy, coneHalfAngle);
	//voxelcolor += cone_tracing(normals.xy + vec2(-.1, -.1), world_pos.xy, coneHalfAngle);
	//voxelcolor /= 5;
	vec3 reflection = cone_tracing(reflect(vec2(0), normals.xy), world_pos.xy, coneHalfAngle);
	float magn = length(voxelcolor);
	color.rgb = texturecolor;
	if (dovoxel > 0.5)
	{
		color.rgb += voxelcolor;// *1.4;
	}
	else if (dovoxel > 1.5 && distance(mouse_pos, world_pos) > .2)
	{
		color.rgb += voxelcolor;
		color.rgb += reflection;
		color.rgb *= 1.2;
	}

	/*color.rgb = world_pos;*/
}