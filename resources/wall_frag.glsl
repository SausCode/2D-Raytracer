#version 430 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;

uniform vec3 campos;

in vec3 fragPos;
in vec2 fragTex;
in vec3 fragNor;
in vec4 fragViewPos;
in vec4 worldPos;
in float deffered_toggle;

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D tex2;

void main()
{
	vec3 normal = normalize(fragNor);
	vec3 texturecolor = texture(tex, fragTex).rgb;
	vec3 normcolor = texture(tex2, fragTex).rgb;
	pos_out = worldPos;
	norm_out = vec4(normcolor, 1);
	color.a=1;
	color.rgb = texturecolor;
}