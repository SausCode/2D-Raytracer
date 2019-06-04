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

layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D tex2;

uniform vec2 resolution;
uniform float t;
uniform vec3 light_pos;

void main()
{
	vec4 texturecolor = texture(tex, fragTex);
	norm_out = vec4(fragNor, 1);
	vec2 cPos = -1.0 + 2.0 * gl_FragCoord.xy / resolution.xy;
	float cLength = length(cPos);
	vec2 uv = gl_FragCoord.xy/resolution.xy+(cPos/cLength)*cos(cLength*12.0-t*4.0)*0.03;
	vec3 col = texture(tex,uv).xyz;
	pos_out = worldPos;
	pos_out.z = 0;
	color.rgb = col;
	color.a = 1;
	mask_out = vec4(color.rgb, 1);
}
