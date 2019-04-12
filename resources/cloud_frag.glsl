#version 430 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;
layout(location = 3) out vec4 mask_out;

in vec3 fragPos;
in vec2 fragTex;
in vec3 fragNor;
in vec4 fragViewPos;
in vec4 worldPos;

uniform vec2 cloud_offset;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D tex2;

 void main()
{
	vec4 texturecolor = texture(tex, fragTex+cloud_offset);
	vec4 normalfromtex = texture(tex2, fragTex+cloud_offset);
	vec3 texturenormal = (normalfromtex.rgb - vec3(0.5, 0.5, 0.5));
	texturenormal = texturenormal * 2.0;
	vec3 ey = normalize(fragNor);
	vec3 ez = vec3(0, 0, 1);
	vec3 ex = cross(ez, ey);
	mat3 TBN = mat3(ex, ey, ez);
	vec3 readynormal = normalize(TBN*texturenormal);
	mask_out = vec4(1,0,0,1);
	pos_out = worldPos;
	color = texturecolor;
	
	readynormal *= 0.25f;
	vec3 blue = vec3(0.498f, 0.498f, 1) *0.75f;
	norm_out = vec4(readynormal, color.a) + vec4(blue, color.a);
}
