#version 430

in vec2 fragTex;

layout(location = 0) out vec4 color;
layout(location = 1) out vec4 pos_out;
layout(location = 2) out vec4 norm_out;
layout(location = 3) out vec4 cloud_mask_out;

layout(location = 0) uniform sampler2D col_tex;
layout(location = 1) uniform sampler2D pos_tex;
layout(location = 2) uniform sampler2D norm_tex;
layout(location = 3) uniform sampler2D cloud_mask_tex;

uniform int passRender;
uniform vec3 mouse_pos;
uniform vec2 cloud_center;
uniform float cloud_radius;
uniform int screen_width;
uniform int screen_height;

struct traceInfo {
	vec3 color;
};

vec2 voxel_transform(vec2 pos)
{
	vec2 texpos = pos + vec2(1, 1);
	texpos /= 2;
	return texpos;
}

vec4 sampling(vec2 conedirection, vec2 texposition, float mipmap, vec2 pixposition, float is_in_cloud)
{
	uint imip = uint(mipmap);
	float linint = mipmap - float(imip);
	vec4 colA = texture(col_tex, texposition.xy, imip);
	vec4 colB = texture(col_tex, texposition.xy, imip + 1);
	float d = 1.0;
	if(is_in_cloud==0)
	{
		vec2 norm = -texture(norm_tex, texposition.xy, 1).xy;
		d = dot(normalize(norm), normalize(conedirection));
		d = clamp(d, 0, 1);
	}
		
	vec4 col = mix(colA, colB, linint)*d;
	return col;
}

traceInfo cone_tracing(vec2 conedirection, vec2 pixelpos, float angle,int stepcount, float is_in_cloud, int passNum)
{
	traceInfo t;
	conedirection = normalize(conedirection);
	float voxelSize = 2. / 256.;
	vec4 trace = vec4(0);
	float distanceFromConeOrigin = voxelSize * 2;
	float coneHalfAngle = angle;
	vec2 texpos;
	vec2 pixelposition;
	for (int i = 0; i < stepcount; i++)
	{
		float coneDiameter = 2 * tan(coneHalfAngle) * distanceFromConeOrigin;
		float mip = log2(coneDiameter / voxelSize);
		pixelposition = pixelpos + conedirection * distanceFromConeOrigin;
		texpos = voxel_transform(pixelposition);
		trace += sampling(conedirection, texpos, mip, pixelposition, is_in_cloud);
		distanceFromConeOrigin += voxelSize*3;
		
		if(	(trace.a>0.25 && is_in_cloud==0) || 
			(trace.a>0.5 && is_in_cloud > 0) &&
			(passNum!=3))	{ break; }
	}
	t.color = trace.rgb;
	return t;
}
mat4 rotationMatrix(vec3 axis, float angle)
{
	axis = normalize(axis);
	float s = sin(angle);
	float c = cos(angle);
	float oc = 1.0 - c;

	return mat4(oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s, oc * axis.z * axis.x + axis.y * s, 0.0,
				oc * axis.x * axis.y + axis.z * s, oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s, 0.0,
				oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s, oc * axis.z * axis.z + c, 0.0,
				0.0, 0.0, 0.0, 1.0);
}
vec4 crossProduct(vec4 A, vec4 B) 
  
{ 
	vec3 crossP=vec3(0); 
    crossP.x = A.y * B.z - A.z * B.y; 
    crossP.y = A.x * B.z - A.z * B.x; 
    crossP.z = A.x * B.y - A.y * B.x;
	return vec4(crossP, 1);
}

float checkAngleWithQuadrant(vec2 point, float angle)
{
	if(point.x>=0 && point.y>=0)
	{
		return angle;
	}
	else if(point.x<0 && point.y>0)
	{
			return 180.0f+angle;
	}
	else if(point.x>0 && point.y<0)
	{
			return 360.0f + angle;
	}
	else
	{
			return 180.0f+angle;
	}

}

float convertRadianToDegree(float radian)
{
	return radian * (180.0f/3.14159625f);
}

void main()
{
	float coneHalfAngle = 1.05; // 60 degree cone!
	vec3 texturecolor = texture(col_tex, fragTex).rgb;
	vec3 normals = texture(norm_tex, fragTex).rgb;
	vec3 world_pos = texture(pos_tex, fragTex).rgb;
	vec4 is_in_cloud = texture(cloud_mask_tex, fragTex);

	color = vec4(texturecolor,1);	

	vec2 lightdirection = normalize(mouse_pos.xy - world_pos.xy);

	if (world_pos != vec3(0))
	{
	
		if (passRender == 2)
		{
			traceInfo t;
			if (is_in_cloud.a == 0)
				t = cone_tracing(normals.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
			else
			{
				t = cone_tracing(lightdirection, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
				t.color *= is_in_cloud.xyz;
			}
			color.rgb += t.color;
		}
		else if (passRender == 3 && is_in_cloud.a > 0)
		{

			mat4 Rz = rotationMatrix(vec3(0, 0, 1), 3.1415926 / 2.);
			traceInfo t1,t2,t3,t4;
			vec4 conedirection = vec4(lightdirection,0,1);
			t1 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
			conedirection = Rz * conedirection;
			t2 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
			conedirection = Rz * conedirection;
			t3 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
			conedirection = Rz * conedirection;
			t4 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
			vec3 colorsum = t1.color.rgb + t2.color.rgb + t3.color.rgb + t4.color.rgb;
			colorsum *= is_in_cloud.xyz;
			color.rgb += colorsum;
		}
		else if(passRender==4 && is_in_cloud.a==0)
		{
//			vec4 Rz = vec4(0, 0, 1,0);
//			traceInfo t1,t2,t3,t4;
//			vec4 conedirection = vec4(normals.xy,0,1);
//			conedirection = crossProduct(Rz, conedirection);
//			t1 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
//			conedirection = crossProduct(Rz, conedirection);
//			t2 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
//			Rz = vec4(0, 0, -1,0);
//			conedirection = crossProduct(Rz, conedirection);
//			t3 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
//			conedirection = crossProduct(Rz, conedirection);
//			t4 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
//			vec3 colorsum = t1.color.rgb + t2.color.rgb + t3.color.rgb + t4.color.rgb;
//			color.rgb += colorsum;

			mat4 Rz = rotationMatrix(vec3(0, 0, 1), 3.1415926 / 2.);
			traceInfo t1,t2,t3,t4;
			float atan1, atan2, atan3, atan4;
			vec4 conedirection = vec4(normals.xy,0,1);
			atan1 = atan(conedirection.y, conedirection.x);
			atan1 = convertRadianToDegree(atan1);
			atan1 = checkAngleWithQuadrant(conedirection.xy, atan1);
			t1 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
			conedirection = Rz * conedirection;
			atan2 = atan(conedirection.y, conedirection.x);
			atan2 = convertRadianToDegree(atan2);
			atan2 = checkAngleWithQuadrant(conedirection.xy, atan2);
			t2 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
			Rz = rotationMatrix(vec3(0, 0, -1), 3.1415926 / 2.);
			conedirection = Rz * conedirection;
			atan3 = atan(conedirection.y, conedirection.x);
			atan3 = convertRadianToDegree(atan3);
			atan3 = checkAngleWithQuadrant(conedirection.xy, atan3);
			t3 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
			conedirection = Rz * conedirection;
			atan4 = atan(conedirection.y, conedirection.x);
			atan4 = convertRadianToDegree(atan4);
			atan4 = checkAngleWithQuadrant(conedirection.xy, atan4);
			t4 = cone_tracing(conedirection.xy, world_pos.xy, coneHalfAngle, 15, is_in_cloud.a, passRender);
			vec3 colorsum = t1.color.rgb + t2.color.rgb + t3.color.rgb + t4.color.rgb;
			color.rgb += colorsum;

		}
	}
	
	

	norm_out = vec4(normals, 1);
	pos_out = vec4(world_pos, 1);
	cloud_mask_out = is_in_cloud;
}