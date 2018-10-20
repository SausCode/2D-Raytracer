#pragma once


#ifndef LAB471_CAMERA_H_INCLUDED
#define LAB471_CAMERA_H_INCLUDED

#include <stack>
#include <memory>

#include "glm/glm.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"





class camera
{
public:
	glm::vec3 pos;
	glm::vec3 rot;
	int w, a, s, d;
	camera()
	{
		w = a = s = d = 0;
		pos = rot = glm::vec3(0, 0, 0);
	}
	glm::mat4 process()
	{
		float going_forward = 0.0;
		if (w == 1)
			going_forward += 0.1;
		if (s == 1)
			going_forward -= 0.1;
		if (a == 1)
			rot.y -= 0.1;
		if (d == 1)
			rot.y += 0.1;
		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));

		glm::vec4 rpos = glm::vec4(0, 0, going_forward, 1);

		rpos = R *rpos;
		pos.x += -rpos.x;
		pos.z += rpos.z;

		glm::mat4 T = glm::translate(glm::mat4(1), glm::vec3(pos.x, pos.y, pos.z));
		return R*T;
	}

};








#endif // LAB471_CAMERA_H_INCLUDED