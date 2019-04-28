//#version 430 core
//
//layout(location = 0) out vec4 color;
//layout(location = 1) out vec4 pos_out;
//layout(location = 2) out vec4 norm_out;
//
//in vec3 fragPos;
//in vec2 fragTex;
//in vec3 fragNor;
//in vec4 fragViewPos;
//in vec4 worldPos;
//
//layout(location = 0) uniform sampler2D tex;
//layout(location = 1) uniform sampler2D tex2;
//
//uniform vec2 resolution;
//uniform float t;
//uniform vec3 light_pos;
//
//void main()
//{
//	vec4 texturecolor = texture(tex, fragTex);
//
//	vec2 displacement = texture(tex2, fragTex/6.0).xy;
//
//	float y = fragTex.y + displacement.y * 0.1 - 0.5 + (sin(fragTex.x * 120.0 + t) * 0.1);
//
//	norm_out = vec4(0,1,1,0);
////	pos_out = worldPos;
//
//	color = texture(tex, vec2 (fragTex.x, y));
//
//	color.a *= color.a;
//
//	float norm_y = (cos(fragTex.x))*1/sqrt(1+cos(fragTex.x)*cos(fragTex.x));
//
//	norm_out.b = norm_y;
//
//	vec2 cPos = -1.0 + 2.0 * gl_FragCoord.xy / resolution.xy;
//	float cLength = length(cPos);
//
//	vec2 uv = gl_FragCoord.xy/resolution.xy+(cPos/cLength)*cos(cLength*12.0-t*4.0)*0.03;
//	vec3 col = texture2D(tex,uv).xyz;
//
//	color.rgb = col;
//	color.a = 1;
//
//	color.rgb = vec3(fragNor);
//
//	color.rgb = vec3(1);
//
//
//	if (fragNor.y > 0)
//		color.rgb = col;
//	else
//	{
//		color.rgb = (.5 * fragNor);
////		color.a = 1-gl_FragCoord.y;
//	}
//
////	color.rgb = fragNor + col;
//
////	color.rgb = fragNor;
//}
//

#version 430 core

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;

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

	vec2 displacement = texture(tex2, fragTex/6.0).xy;

	float y = fragTex.y + displacement.y * 0.1 - 0.5 + (sin(fragTex.x * 120.0 + t) * 0.1);

	norm_out = vec4(fragNor,1);
//	pos_out = worldPos;

	color = texture(tex, vec2 (fragTex.x, y));

	color.a *= color.a;

//	float norm_y = (cos(fragTex.x))*1/sqrt(1+cos(fragTex.x)*cos(fragTex.x));

//	norm_out.b = norm_y;

	vec2 cPos = -1.0 + 2.0 * gl_FragCoord.xy / resolution.xy;
	float cLength = length(cPos);

	vec2 uv = gl_FragCoord.xy/resolution.xy+(cPos/cLength)*cos(cLength*12.0-t*4.0)*0.03;
	vec3 col = texture2D(tex,uv).xyz;

	pos_out = worldPos;

	color.rgb = col;
	color.a = .5;

//	color.rgb = vec3(fragNor);

//	color.rgb = vec3(gl_FragCoord.y);
//	color.rgb = vec3(1);

//	color.rgb = vec3(worldPos.z);

//	color.rgb = worldPos.xyz;


//	if (fragNor.y > 0)
//		color.rgb = col;
//	else
//	{
//		color.rgb = (.5 * fragNor);
////		color.a = 1-gl_FragCoord.y;
//	}

//	color.rgb = fragNor + col;

//	color.rgb = fragNor;
}
