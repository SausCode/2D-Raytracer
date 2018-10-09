#version 450 core
#define NUM_LIGHTS 100

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 normal_out;
layout(location = 2) out vec4 viewpos;
layout(location = 3) out vec4 bloom_tex;

uniform vec3 campos;

in vec3 fragPos;
in vec2 fragTex;
in vec3 fragNor;
in vec4 fragViewPos;
in vec4 worldPos;
in float deffered_toggle;

layout(location = 0) uniform sampler2D tex;

vec3 lights[NUM_LIGHTS] = {vec3(0, 0, 0), vec3(1, 1, 1), vec3(2, 2, 2), vec3(3, 3, 3), vec3(4, 4, 4), vec3(5, 5, 5), vec3(6, 6, 6), vec3(7, 7, 7), vec3(8, 8, 8), vec3(9, 9, 9), vec3(10, 10, 10), vec3(11, 11, 11), vec3(12, 12, 12), vec3(13, 13, 13), vec3(14, 14, 14), vec3(15, 15, 15), vec3(16, 16, 16), vec3(17, 17, 17), vec3(18, 18, 18), vec3(19, 19, 19), vec3(20, 20, 20), vec3(21, 21, 21), vec3(22, 22, 22), vec3(23, 23, 23), vec3(24, 24, 24), vec3(25, 25, 25), vec3(26, 26, 26), vec3(27, 27, 27), vec3(28, 28, 28), vec3(29, 29, 29), vec3(30, 30, 30), vec3(31, 31, 31), vec3(32, 32, 32), vec3(33, 33, 33), vec3(34, 34, 34), vec3(35, 35, 35), vec3(36, 36, 36), vec3(37, 37, 37), vec3(38, 38, 38), vec3(39, 39, 39), vec3(40, 40, 40), vec3(41, 41, 41), vec3(42, 42, 42), vec3(43, 43, 43), vec3(44, 44, 44), vec3(45, 45, 45), vec3(46, 46, 46), vec3(47, 47, 47), vec3(48, 48, 48), vec3(49, 49, 49), vec3(50, 50, 50), vec3(51, 51, 51), vec3(52, 52, 52), vec3(53, 53, 53), vec3(54, 54, 54), vec3(55, 55, 55), vec3(56, 56, 56), vec3(57, 57, 57), vec3(58, 58, 58), vec3(59, 59, 59), vec3(60, 60, 60), vec3(61, 61, 61), vec3(62, 62, 62), vec3(63, 63, 63), vec3(64, 64, 64), vec3(65, 65, 65), vec3(66, 66, 66), vec3(67, 67, 67), vec3(68, 68, 68), vec3(69, 69, 69), vec3(70, 70, 70), vec3(71, 71, 71), vec3(72, 72, 72), vec3(73, 73, 73), vec3(74, 74, 74), vec3(75, 75, 75), vec3(76, 76, 76), vec3(77, 77, 77), vec3(78, 78, 78), vec3(79, 79, 79), vec3(80, 80, 80), vec3(81, 81, 81), vec3(82, 82, 82), vec3(83, 83, 83), vec3(84, 84, 84), vec3(85, 85, 85), vec3(86, 86, 86), vec3(87, 87, 87), vec3(88, 88, 88), vec3(89, 89, 89), vec3(90, 90, 90), vec3(91, 91, 91), vec3(92, 92, 92), vec3(93, 93, 93), vec3(94, 94, 94), vec3(95, 95, 95), vec3(96, 96, 96), vec3(97, 97, 97), vec3(98, 98, 98), vec3(99, 99, 99)};

void main()
{
	vec3 normal = normalize(fragNor);
	vec3 texturecolor = texture(tex, fragTex).rgb;
	viewpos = worldPos;
	normal_out = vec4(normal, 1);
	color.a=1;

	if (deffered_toggle > .5){
		color.rgb = texturecolor;

		if (color.r > .7 && color.g > .7 && color.b > .7){
			bloom_tex.rgb = color.rgb;
			bloom_tex.a = 1;
		}
	}
	else{
		for (int i = 0; i < NUM_LIGHTS; i++){
			vec3 lp = lights[i];
			vec3 ld = normalize(lp - worldPos.xyz);
			float light = dot(ld,normal);	
			light = clamp(light,0,1);

			//specular light
			vec3 camvec = normalize(campos - worldPos.xyz);
			vec3 h = normalize(camvec+ld);
			float spec = pow(dot(h,normal),5);
			spec = clamp(spec,0,1)*0.3;
	
			color.rgb += texturecolor *light + vec3(1,1,1)*spec;
		}

		color.rgb /= NUM_LIGHTS;
	}
}
