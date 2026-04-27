#pragma once

#include <glm/glm.hpp>

#include "Log.h"

static void LogMat(const glm::mat4 &mat)
{
	Log::Info("{:.2f}, {:.2f}, {:.2f}, {:.2f}", mat[0][0], mat[0][1], mat[0][2], mat[0][3]);
	Log::Info("{:.2f}, {:.2f}, {:.2f}, {:.2f}", mat[1][0], mat[1][1], mat[1][2], mat[1][3]);
	Log::Info("{:.2f}, {:.2f}, {:.2f}, {:.2f}", mat[2][0], mat[2][1], mat[2][2], mat[2][3]);
	Log::Info("{:.2f}, {:.2f}, {:.2f}, {:.2f}", mat[3][0], mat[3][1], mat[3][2], mat[3][3]);
}

static void LogVec3(const glm::vec3 &vec)
{
	Log::Info("{:.2f}, {:.2f}, {:.2f}", vec.x, vec.y, vec.z);
}

static void LogVec4(const glm::vec4 &vec)
{
	Log::Info("{:.2f}, {:.2f}, {:.2f}, {:.2f}", vec.x, vec.y, vec.z, vec.w);
}
