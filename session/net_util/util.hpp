#pragma once

#include <cstdint>

namespace net
{

	namespace util
	{
		// 从包头获取包体的长度
		template<typename _Tp> uint32_t get_body_size(const _Tp& head) { return head; }
	}
}
