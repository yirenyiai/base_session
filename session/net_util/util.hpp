#pragma once

#include <cstdint>

namespace net
{

	namespace util
	{
		// �Ӱ�ͷ��ȡ����ĳ���
		template<typename _Tp> uint32_t get_body_size(const _Tp& head) { return head; }
	}
}
