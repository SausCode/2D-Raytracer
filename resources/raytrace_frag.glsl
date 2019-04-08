#version 430
out vec4 color;
in vec2 fragTex;

layout(location = 0) uniform sampler2D col_tex;
layout(location = 1) uniform sampler2D pos_tex;
layout(location = 2) uniform sampler2D norm_tex;
layout(location = 3) uniform sampler2D no_light_tex;

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

int is_in_cloud(vec2 pix_pos, vec2 center, float radius)
{
	if(pix_pos==vec2(0))
		return 0;

	vec2 trans_center = center;
	trans_center.x /= (10.0f);
	trans_center.y /= (10.0f);

	float trans_radius = (radius+1.1f)/(10.0f);

	vec2 dist = pix_pos - trans_center;
	vec2 top_sq =  pow(dist, vec2(2.0f));
	float r_sq = pow(trans_radius, 2.0f);
	float c = ((top_sq.x)/r_sq) + ((top_sq.y)/r_sq);
	if(c<=1.0)
		return 1;
	else
		return 0;
}

vec4 sampling(vec2 conedirection, vec2 texposition, float mipmap, vec2 pixposition)
{
	uint imip = uint(mipmap);
	float linint = mipmap - float(imip);
	vec4 colA = texture(col_tex, texposition.xy, imip);
	vec4 colB = texture(col_tex, texposition.xy, imip + 1);
	float d = 1.0;
	if(is_in_cloud(pixposition, cloud_center, cloud_radius)==0)
	{
		vec2 norm = -texture(norm_tex, texposition.xy, 1).xy;
		d = dot(normalize(norm), normalize(conedirection));
		d = clamp(d, 0, 1);
	}
		
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
	vec2 pixelposition;
	for (int i = 0; i < 15; i++)
	{
		float coneDiameter = 2 * tan(coneHalfAngle) * distanceFromConeOrigin;
		float mip = log2(coneDiameter / voxelSize);
		pixelposition = pixelpos + conedirection * distanceFromConeOrigin;
		texpos = voxel_transform(pixelposition);
		trace += sampling(conedirection, texpos, mip, pixelposition);
		distanceFromConeOrigin += voxelSize;
		if((trace.a>0.25) && (is_in_cloud(pixelpos, cloud_center, cloud_radius)==0)){
			t.color = vec3(1,0,0);
			return t;
		//	break;
		}
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
		switch (pass)
		{
			case 1:
				color.rgb = texturecolor;
				break;
			case 2:
				color.rgb += multi_bounce(normals.xy, world_pos.xy, coneHalfAngle, 1);
				break;
		}
	}
	else
		color.rgb = texturecolor;

}