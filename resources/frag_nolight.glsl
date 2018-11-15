#version 450 core 

out vec4 color;

in vec2 fragTex;


layout(location = 0) uniform sampler2D tex;
layout(location = 1) uniform sampler2D texnorm;
layout(location = 2) uniform sampler2D texpos;
//layout(binding = 2,rgba16f) uniform image3D vox_output;
layout(binding = 3) uniform sampler3D vox_output0;
layout(binding = 4) uniform sampler3D vox_output1;
layout(binding = 5) uniform sampler3D vox_output2;
layout(binding = 6) uniform sampler3D vox_output3;
layout(binding = 7) uniform sampler3D vox_output4;
layout(binding = 8) uniform sampler3D vox_output5;
layout(binding = 9) uniform sampler3D vox_output6;

vec3 voxel_transform(vec3 pos)
	{
	//sponza is in the frame of [-10,10], and we have to map that to [0,255] in x, y, z
	vec3 texpos= pos + vec3(10,10,10);	//[0,20]
	texpos /= 20.;				//[0,1]
	//pos*=255;				//[0,255]
	//ivec3 ipos = ivec3(pos);
	return texpos;
	}
vec4 getmimappedcolor(vec3 texposition,uint mip)
{
if(mip ==0) return texture(vox_output0, texposition);
else if(mip == 1) return texture(vox_output1, texposition);
else if(mip == 2) return texture(vox_output2, texposition);
else if(mip == 3) return texture(vox_output3, texposition);
else if(mip == 4) return texture(vox_output4, texposition);
else if(mip == 5) return texture(vox_output5, texposition);
else			  return texture(vox_output6, texposition);
return vec4(0,0,0,0);
}
vec4 sampling(vec3 texposition,float mipmap)
{
uint imip = uint(mipmap);
float linint = mipmap - float(imip);
vec4 colA = getmimappedcolor(texposition,imip);
vec4 colB = getmimappedcolor(texposition,imip+1);
return colA * (1-linint) + colB*linint;
}

vec3 cone_tracing(vec3 conedirection,vec3 pixelpos)
{
conedirection=normalize(conedirection);
float voxelSize = 20./256.;				//[-10,10] / resolution	
//pixelpos += conedirection*voxelSize;	//to get some distance to the pixel against self-inducing
vec4 trace = vec4(0);
float distanceFromConeOrigin = voxelSize*2;
float coneHalfAngle = 0.471239; //27 degree
for(int i=0;i<10;i++)
	{
	float coneDiameter = 2 * tan(coneHalfAngle) * distanceFromConeOrigin;
	float mip = log2(coneDiameter / voxelSize);
	pixelpos = pixelpos + conedirection*distanceFromConeOrigin;
	vec3 texpos = voxel_transform(pixelpos);
	trace +=sampling(texpos,mip);
	//return trace.rgb;
	if(trace.a>0.7)break;
	distanceFromConeOrigin+=voxelSize;
	}
return trace.rgb;
}
uniform float manualmipmaplevel;

void main()
{

	//vec3 voxtexturecolor = texture(vox_output3, vec3(fragTex,0.25)).rgb;
	vec3 texturecolor = texture(tex, fragTex).rgb;
	vec3 normal = texture(texnorm, fragTex).rgb;
	vec3 worldpos = texture(texpos, fragTex).rgb;
	//tracing in normal direction:
	
	vec3 texpos = voxel_transform(worldpos);
	vec3 voxelcolor;// = texture(vox_output, texpos,manualmipmaplevel).rgb;
	voxelcolor = cone_tracing(normal,worldpos);

	float magn = length(voxelcolor);
	color.rgb = texturecolor+voxelcolor;
	
	color.a = 1.0;
}
