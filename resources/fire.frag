#version 430 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;
layout(location = 3) out vec4 cloud_mask_out;
layout(location = 4) out vec4 water_mask_out;

in vec3 fragNor;
in vec4 pos;
in vec2 fragTex;

uniform sampler2D tex;
uniform float t;
uniform vec2 to;
uniform vec2 to2;

void main()
{
	vec2 new_tex = vec2(fragTex.x/8.0, fragTex.y/8.0);

	vec4 tcol1 = (1 - t) * texture(tex, new_tex + to);
	vec4 tcol2 = t * texture(tex, new_tex + to2);

	color = tcol1 + tcol2;
	if(color.a<0.1)
		discard;

	pos_out = pos;

	cloud_mask_out = vec4(color.rgb,0);
	norm_out = vec4(normalize(fragNor), 1);
	water_mask_out = vec4(color.rgb, 0);

}