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

layout(location = 0) uniform sampler2D dudvtex;
layout(location = 1) uniform sampler2D normtex;

//uniform vec2 resolution;
//uniform float t;
//uniform vec3 light_pos;

uniform float moveFactor1;
uniform float moveFactor2;

void main()
{
//	vec4 texturecolor = texture(tex, fragTex);
//	norm_out = vec4(fragNor, 1);
//	vec2 cPos = -1.0 + 2.0 * gl_FragCoord.xy / resolution.xy;
//	float cLength = length(cPos);
//	vec2 uv = gl_FragCoord.xy/resolution.xy+(cPos/cLength)*cos(cLength*12.0-t*4.0)*0.03;
//	vec3 col = texture(tex,uv).xyz;
//	pos_out = worldPos;
//	pos_out.z = 0;
//	color.rgb = col;
//	color.a = 1;
//	mask_out = vec4(color.rgb, 1);

	vec2 distortedTexCoords = texture(dudvtex, vec2(fragTex.x + moveFactor1, fragTex.y)).rg;
	distortedTexCoords = fragTex + vec2(distortedTexCoords.x,distortedTexCoords.y+moveFactor1);
	vec2 totalDistortion = texture(dudvtex, distortedTexCoords).rg*2.0 - 1.0;
	vec4 normalMapColor = texture(normtex, distortedTexCoords);
	vec3 texturenormal = (normalMapColor.rgb - vec3(0.5, 0.5, 0.5));
	texturenormal = texturenormal * 2.0;
	vec3 ey = normalize(fragNor);
	vec3 ez = vec3(0, 0, 1);
	vec3 ex = cross(ez, ey);
	mat3 TBN = mat3(ex, ey, ez);
	vec3 readynormal = normalize(TBN*texturenormal);
	color = vec4(totalDistortion, 1, 1);
	//color.rg += totalDistortion;
	color = clamp(color, 0,1);
	color = mix(color, vec4(0.0,0.3,0.5,1.0), 0.2);
	pos_out = worldPos;
	norm_out = vec4(readynormal, 1);
	
//	vec4 normal = vec4(normalMapColor.r*2.0 - 1.0, normalMapColor.b,normalMapColor.g*2.0 - 1.0, 1.0);
//	norm_out = normalize(normal);
	mask_out = vec4(color.rgb, 0);

}
