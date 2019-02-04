#version 430 core

layout(location = 0) out vec4 color;

in vec3 fragNor;
in vec3 pos;
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
}