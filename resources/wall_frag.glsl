#version 430 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;
layout(location = 3) out vec4 cloud_mask_out;
layout(location = 4) out vec4 water_mask_out;


in vec3 fragPos;
in vec2 fragTex;
in vec3 fragNor;
in vec4 fragViewPos;
in vec4 worldPos;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D tex2;

void main()
{
	vec4 texturecolor = texture(tex, fragTex);
	vec3 normalfromtex = texture(tex2, fragTex).rgb;
	vec3 texturenormal = (normalfromtex - vec3(0.5, 0.5, 0.5));
	texturenormal = texturenormal * 2.0;
	vec3 ey = normalize(fragNor);
	vec3 ez = vec3(0, 0, 1);
	vec3 ex = cross(ez, ey);
	mat3 TBN = mat3(ex, ey, ez);
	vec3 readynormal = normalize(TBN*texturenormal);
	cloud_mask_out = vec4(texturecolor.rgb,0);
	pos_out = worldPos;
	norm_out = vec4(readynormal, 1);
	norm_out = vec4(normalize(fragNor), 1);
	water_mask_out = vec4(color.rgb, 0);

	
	color = texturecolor;
	if(color.a==0)
		discard;
}
