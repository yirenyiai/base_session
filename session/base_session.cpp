/*
 * base_session.cpp
 *
 *  Created on: Mar 9, 2017
 *      Author: yirenyiai
 */
#include "base_session.h"

namespace net
{
	base_session::base_session(event_base* ebase)
		: m_keep_alive_second(30)
		, m_ebase(ebase)
		, m_time_reconnect_ebase(NULL)    
		, m_keep_alive_ebase(NULL)
		, m_bufevserver(NULL) 			
		, m_upstream_port(0)
		, m_login_status(false)
		, m_dely_second(10)
		, m_max_dely_second(120)
	{
		m_bufevserver = bufferevent_socket_new(m_ebase, -1, BEV_OPT_CLOSE_ON_FREE);

		m_time_reconnect_ebase = evtimer_new(m_ebase, base_session::on_reconnect_upstream, this);
		m_keep_alive_ebase  = evtimer_new(m_ebase, base_session::on_keep_alive_event, this);
	}

	base_session::~base_session()
	{
		if (m_time_reconnect_ebase)
			event_free(m_time_reconnect_ebase);
		if (m_keep_alive_ebase)
			event_free(m_keep_alive_ebase);
	}

	void base_session::set_upstream_ip(const std::string& ip)
	{
		m_upstream_ip = ip;
	}

	void base_session::set_upstream_port(const uint32_t& port)
	{
		m_upstream_port = port;
	}

	int base_session::run()
	{
		assert(!m_upstream_ip.empty() && m_upstream_port > 1024);
		printf("链接到服务器: %s:%d\n", m_upstream_ip.c_str(), m_upstream_port);

		sockaddr_in sin;
		memset(&sin, 0, sizeof(sockaddr_in));
		sin.sin_family 		= AF_INET;
		sin.sin_addr.s_addr = inet_addr(m_upstream_ip.c_str());
		sin.sin_port 		= htons(m_upstream_port);

		bufferevent_enable(m_bufevserver, EV_READ | EV_WRITE);
		bufferevent_setcb(m_bufevserver, base_session::readcb, base_session::writecb, base_session::eventcb, this);
		if (bufferevent_socket_connect(m_bufevserver, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		{
			printf("libevent 连接到服务器失败\n");
			delay_connect();
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}

	void base_session::write_data(const void* data, size_t size)
	{
		bool bEnable = EV_WRITE & bufferevent_get_enabled(m_bufevserver);
		if (bEnable)
		{
			assert(size > 0);
			if (0 != bufferevent_write(m_bufevserver, data, size))
			{
				printf("base_session::send_req  发送请求失败, 本次丢失的数据由 :%d bytes\n", size);
			}
		}
	}

	void base_session::delay_connect()
	{
		// 释放socket资源
		release_socket();

		timeval tv;
		tv.tv_sec = m_dely_second;
		tv.tv_usec = 0;

		// 设置定时器
		evtimer_add(m_time_reconnect_ebase, &tv);

		// 扩大延迟
		if (m_dely_second < m_max_dely_second)
			m_dely_second += 10;
	}

	void base_session::on_event(bufferevent *bev, short what)
	{
		if (what & BEV_EVENT_CONNECTED)
		{
			m_dely_second = 10;		// 重置重连延迟
		}
		else
		{
			if (what & BEV_EVENT_EOF)
			{
				printf("base_session::on_event : socket 链接接收到一个EOF退出指令, 系统正常推出\n");
			}
			else
			{
				// 发生错误， 则持续连接
				if (what & BEV_EVENT_ERROR)
				{
					int err = bufferevent_socket_get_dns_error(bev);
					if (err)
					{
						printf("base_session::on_event : 连接到服务器失败，错误码 : %d\n", err);
					}
				}
				else if (what & BEV_EVENT_TIMEOUT)
				{
					printf("base_session::on_event : socket 连接超时,将重新进行重新连接\n");
				}
				else
				{
					printf("base_session::on_event : socket 出错：%d\n", what);
				}
			}
		}
	}

	void base_session::on_keep_alive()
	{

		// 重置定时器
		timeval tv;
		tv.tv_sec = m_keep_alive_second;
		tv.tv_usec = 0;

		evtimer_add(m_keep_alive_ebase, &tv);

	}

	// 与数据服务器连接事件回调
	void base_session::eventcb(bufferevent *bev, short what, void *ctx)
	{
		base_session* pthis = static_cast<base_session*>(ctx);
		assert(pthis);
		pthis->on_event(bev, what);
	}


	// 数据服务器回应到达回调
	void base_session::readcb(bufferevent *bev, void *ctx)
	{
		base_session* pthis = static_cast<base_session*>(ctx);
		pthis->on_read(bev);
	}

	void base_session::writecb(struct bufferevent *bev, void *ctx)
	{
		base_session* pthis = static_cast<base_session*>(ctx);
		pthis->on_write(bev);
	}

	void base_session::on_keep_alive_event(evutil_socket_t fd, short libev, void *ctx)
	{
		base_session* pthis = static_cast<base_session*>(ctx);
		assert(pthis);
		pthis->on_keep_alive();
	}

	void base_session::on_reconnect_upstream(evutil_socket_t fd, short libev, void *ctx)
	{
		base_session* pthis = static_cast<base_session*>(ctx);
		printf("重新链接到行情服务器\n");

		if (EXIT_FAILURE == pthis->run())
			pthis->delay_connect();
	}


	void base_session::release_socket()
	{
		// 关闭呼吸包定时器
		assert(m_keep_alive_ebase);
		evtimer_del(m_keep_alive_ebase);

		// 关闭Socket句柄
		evutil_socket_t fd = bufferevent_getfd(m_bufevserver);
		if (fd != -1)
		{
			bufferevent_setfd(m_bufevserver, -1);
			evutil_closesocket(fd);
		}

		// 释放内存
		struct evbuffer* input = bufferevent_get_input(m_bufevserver);
		if (input)
		{
			const size_t input_len = evbuffer_get_length(input);
			evbuffer_drain(input, input_len);
		}

		struct evbuffer* output = bufferevent_get_output(m_bufevserver);
		if (output)
		{
			const size_t output_len = evbuffer_get_length(output);
			evbuffer_drain(output, output_len);
		}

		// 重置登录状态
		m_login_status = false;
	}
}
