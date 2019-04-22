#version 430

in vec2 fragTex;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;
layout(location = 3) out vec4 mask_out;


layout(location = 0) uniform sampler2D col_tex;
layout(location = 1) uniform sampler2D pos_tex;
layout(location = 2) uniform sampler2D norm_tex;
layout(location = 3) uniform sampler2D mask_tex;

uniform int pass;
uniform vec3 mouse_pos;
uniform vec2 cloud_center;
uniform float cloud_radius;
uniform int screen_width;
uniform int screen_height;

struct traceInfo {
	vec3 color;
	vec2 pos;
};

vec2 voxel_transform(vec2 pos)
{
	vec2 texpos = pos + vec2(1, 1);
	texpos /= 2;
	return texpos;
}

vec4 sampling(vec2 conedirection, vec2 texposition, float mipmap, vec2 pixposition, float is_in_cloud)
{
	uint imip = uint(mipmap);
	float linint = mipmap - float(imip);
	vec4 colA = texture(col_tex, texposition.xy, imip);
	vec4 colB = texture(col_tex, texposition.xy, imip + 1);
	float d = 1.0;
	if(is_in_cloud==0)
	{
		vec2 norm = -texture(norm_tex, texposition.xy, 1).xy;
		d = dot(normalize(norm), normalize(conedirection));
		d = clamp(d, 0, 1);
	}
		
	vec4 col = mix(colA, colB, linint)*d;
	return col;
}

traceInfo cone_tracing(vec2 conedirection, vec2 pixelpos, float angle,int stepcount, float is_in_cloud)
{
	traceInfo t;
	conedirection = normalize(conedirection);
	float voxelSize = 2. / 256.;
	vec4 trace = vec4(0);
	float distanceFromConeOrigin = voxelSize * 2;
	float coneHalfAngle = angle;
	vec2 texpos;
	vec2 pixelposition;
	for (int i = 0; i < stepcount; i++)
	{
		float coneDiameter = 2 * tan(coneHalfAngle) * distanceFromConeOrigin;
		float mip = log2(coneDiameter / voxelSize);
		pixelposition = pixelpos + conedirection * distanceFromConeOrigin;
		texpos = voxel_transform(pixelposition);
		trace += sampling(conedirection, texpos, mip, pixelposition, is_in_cloud);
		distanceFromConeOrigin += voxelSize*3;
		if((trace.a>0.25) && is_in_cloud==0){
			break;
		}
	}
	t.color = trace.rgb;
	t.pos = texpos;
	return t;
}

void main()
{
	color.a = 1;
	float coneHalfAngle = 0.25;
	vec3 texturecolor = texture(col_tex, fragTex).rgb;
	vec3 normals = texture(norm_tex, fragTex).rgb;
	vec3 world_pos = texture(pos_tex, fragTex).rgb;
	vec4 is_in_cloud = texture(mask_tex, fragTex);
	vec3 voxelcolor;
	color.rgb = texturecolor;
	
	vec2 lightdirection = normalize(mouse_pos.xy - world_pos.xy);

	if (world_pos != vec3(0))
	{
		switch (pass)
		{
			case 1:
				color.rgb = texturecolor;
				break;
			case 2:
			{
			traceInfo t;
			if(is_in_cloud.a==0)
				t = cone_tracing(normals.xy, world_pos.xy, coneHalfAngle, 15,is_in_cloud.a);
			else
				{
				t = cone_tracing(lightdirection, world_pos.xy, coneHalfAngle*5, 20, is_in_cloud.a);
				t.color *= is_in_cloud.xyz;
				}
			color.rgb += t.color;
			}
				break;
			case 3:
				color.rgb=texturecolor;
				break;
		}
	}
	else
		color.rgb = texturecolor;
	
	if(pass<3)
	{
		norm_out = vec4(normals, 1);
		pos_out = vec4(world_pos, 1);
		mask_out = is_in_cloud;
	}
}