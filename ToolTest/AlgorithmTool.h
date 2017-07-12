#pragma once

namespace algorithm_tool
{
	template<class T>
	float GetDisBy2D(const T & cur_point, const T & target_point)
	{
		auto sum = pow(cur_point.X - target_point.X, 2.0f) + pow(cur_point.Y - target_point.Y, 2.0f);
		return sqrt(sum);
	}
}