#version 430
out vec4 color;
in vec2 fragTex;

layout(location = 0) uniform sampler2D col_tex;
layout(location = 1) uniform sampler2D pos_tex;
layout(location = 2) uniform sampler2D norm_tex;
layout(location = 3) uniform sampler2D no_light_tex;

uniform int dovoxel;
uniform vec3 mouse_pos;

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

vec4 sampling(vec2 conedirection, vec2 texposition, float mipmap)
{
	uint imip = uint(mipmap);
	float linint = mipmap - float(imip);
	vec4 colA = texture(col_tex, texposition.xy, imip);
	vec4 colB = texture(col_tex, texposition.xy, imip + 1);
	vec2 norm = -texture(norm_tex, texposition.xy, 1).xy;
	float d = dot(normalize(norm), normalize(conedirection));
	d = clamp(d, 0, 1);
	vec4 col = mix(colA, colB, linint)*d;
	return col;
}

traceInfo cone_tracing(vec2 conedirection, vec2 pixelpos, float angle)
{
	traceInfo t;
	conedirection = normalize(conedirection);
	float voxelSize = 2. / 256.;
	voxelSize = 0.01;
	vec4 trace = vec4(0);
	float distanceFromConeOrigin = voxelSize * 2;
	float coneHalfAngle = angle;
	vec2 texpos;
	for (int i = 0; i < 15; i++)
	{
		float coneDiameter = 2 * tan(coneHalfAngle) * distanceFromConeOrigin;
		float mip = log2(coneDiameter / voxelSize);
		pixelpos = pixelpos + conedirection * distanceFromConeOrigin;
		texpos = voxel_transform(pixelpos);
		trace += sampling(conedirection, texpos, mip);
		distanceFromConeOrigin += voxelSize;
	}
	t.color = trace.rgb;
	t.pos = texpos;
	return t;
}

vec3 multi_bounce(vec2 conedirection, vec2 pixelpos, float angle, float bounces)
{
	vec3 col;
	traceInfo t;
	t = cone_tracing(conedirection, pixelpos, angle);
	col += t.color;
	for (int i = 1; i < bounces; i++)
	{
		vec2 norm = texture(norm_tex, t.pos).rg;
		col += cone_tracing(norm, t.pos, angle).color;
	}
	return col / bounces;
}

void main()
{
	color.a = 1;
	float coneHalfAngle = 0.05;
	vec3 texturecolor = texture(col_tex, fragTex).rgb;
	vec3 normals = texture(norm_tex, fragTex).rgb;
	vec3 world_pos = texture(pos_tex, fragTex).rgb;
	vec3 voxelcolor;
	color.rgb = texturecolor;
	if (world_pos != vec3(0))
	{
		switch (dovoxel)
		{
			case 0:
				color.rgb = texturecolor;
				break;
			case 1:
				color.rgb += multi_bounce(normals.xy, world_pos.xy, coneHalfAngle, 1);
				break;
			case 2:
				color.rgb += multi_bounce(normals.xy, world_pos.xy, coneHalfAngle, 2);
				break;
			case 3:
				color.rgb += multi_bounce(normals.xy, world_pos.xy, coneHalfAngle, 2);
				color.rgb += multi_bounce(normals.xy + clamp(noise2(vec2(.1, .1)), -.1, .1), world_pos.xy, coneHalfAngle, 1);
				color.rgb += multi_bounce(normals.xy + clamp(noise2(vec2(.1, -.1)), -.1, .1), world_pos.xy, coneHalfAngle, 1);
				color.rgb += multi_bounce(normals.xy + clamp(noise2(vec2(-.1, .1)), -.1, .1), world_pos.xy, coneHalfAngle, 1);
				color.rgb += multi_bounce(normals.xy + clamp(noise2(vec2(-.1, -.1)), -.1, .1), world_pos.xy, coneHalfAngle, 1);
				break;
			case 4:
				color.rgb += multi_bounce(normals.xy, world_pos.xy, coneHalfAngle, 2);
				color.rgb += multi_bounce(normals.xy + clamp(noise2(vec2(.1, .1)), -.1, .1), world_pos.xy, coneHalfAngle, 2);
				color.rgb += multi_bounce(normals.xy + clamp(noise2(vec2(.1, -.1)), -.1, .1), world_pos.xy, coneHalfAngle, 2);
				color.rgb += multi_bounce(normals.xy + clamp(noise2(vec2(-.1, .1)), -.1, .1), world_pos.xy, coneHalfAngle, 2);
				color.rgb += multi_bounce(normals.xy + clamp(noise2(vec2(-.1, -.1)), -.1, .1), world_pos.xy, coneHalfAngle, 2);
				break;
		}
	}
	else
	{
		color.rgb = texturecolor;
	}
}