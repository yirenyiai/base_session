/*
 * base_decoder.h
 *
 *  Created on: Mar 9, 2017
 *      Author: yirenyiai 
 */

#ifndef CLIENT_NET_BASE_DECODER_HPP_
#define CLIENT_NET_BASE_DECODER_HPP_

#include "net_util/util.hpp"

#include <event/event.h>

#include <cstdint>
#include <vector>
#include <cassert>

namespace net
{
	// 基础解码器, 解析二进制协议, 协议格式为 : [固定协议头+包体]
	template<typename _head>
	class base_decoder
	{
	public:
		typedef _head head_type;		
		const static uint32_t head_size = sizeof(head_type);							// 记录包头的长度
		typedef void (*decoder_cb)(typename head_type&, const uint8_t* body_cache, void* ctx);// 接到一个完整数据包
	public:
		void push_back(bufferevent *bev)
		{
			evbuffer_add_buffer(m_cache_evbuf, bufferevent_get_input(bev));
			int real_buf_size = evbuffer_get_length(m_cache_evbuf);
			if (real_buf_size == 0)
				return ;
			do
			{
				if (real_buf_size >= m_need_size)
				{
					if (!m_head_ready)
					{
						int ret_remove = evbuffer_remove(m_cache_evbuf, &m_package_head, head_size);		// 获取包头
						if (ret_remove == -1)
							continue;

						// 如果包体长度为0， 则持续接收下一个数据包
						const uint32_t body_size = get_body_size<head_type>(m_package_head);
						m_head_ready = body_size > 0 ? true : false;
						m_need_size  = m_head_ready ? body_size : head_size;

						if (m_head_ready)
							m_body_cache.resize(body_size);
					}
					else
					{
						int ret_remove = evbuffer_remove(m_cache_evbuf, m_body_cache.data(), m_need_size);
						if (ret_remove == -1)
							continue;

						// 完成了一次包体的接收了, 返回到主线程中执行
						if (m_decoder_cb) 
							m_decoder_cb(m_package_head, m_body_cache.data(), m_ctx);

						// 接收下一个数据包
						m_head_ready = false;
					}
					real_buf_size = evbuffer_get_length(m_cache_evbuf);
				}
				else
					break;
			}while(true);
		}

		base_decoder()
			: m_cache_evbuf(evbuffer_new())
			, m_head_ready(false)
			, m_decoder_cb(NULL)
			, m_ctx(NULL)
		{
		}

		~base_decoder()
		{
			if (m_cache_evbuf)
				evbuffer_free(m_cache_evbuf);
		}

		void set_deocer_cb(decoder_cb cb, void* ctx)
		{
			m_decoder_cb = cb;
			m_ctx = ctx;
		}
	private:
		// 用户数据包解析
		evbuffer* m_cache_evbuf;

		bool m_head_ready;		// 继续是否接到到包头的状态
		head_type m_package_head;			// 包头
		std::vector<uint8_t> m_body_cache;	// 包体的缓冲

		decoder_cb m_decoder_cb; // 回调
		void* m_ctx;			 // 回调携带的上下文消息
	};
}


#endif /* CLIENT_NET_BASE_DECODER_HPP_ */
