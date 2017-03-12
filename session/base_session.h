/*
 * session.h
 *
 *  Created on: Mar 9, 2017
 *      Author: yirenyiai
 */

#ifndef SERVICE_DATACENTER_DOWNLOAD_BASE_SESSION_H_
#define SERVICE_DATACENTER_DOWNLOAD_BASE_SESSION_H_

#include <event/event.h>
#include <cassert>
#include <string>

namespace net
{
		class base_session
		{
		public:
			base_session(event_base* ebase);
			virtual ~base_session();

			void set_upstream_ip(const std::string& ip);
			void set_upstream_port(const uint32_t& port);

			virtual int run();
			virtual void write_data(const void* data, size_t size);

		protected:
			virtual void delay_connect();

			virtual void on_event(bufferevent *bev, short what);
			virtual void on_read(bufferevent *bev) = 0;
			virtual void on_write(bufferevent *bev) = 0;
			virtual void on_keep_alive();

			uint32_t  m_keep_alive_second;
		private:
			// libevent callback function
			static void eventcb(bufferevent *bev, short what, void *ctx);
			static void readcb(bufferevent *bev, void *ctx);
			static void writecb(bufferevent *bev, void *ctx);
			static void on_keep_alive_event(evutil_socket_t fd, short libev, void *ctx);
			static void on_reconnect_upstream(evutil_socket_t fd, short libev, void *ctx);
		private:
			void release_socket();
		private:
			event_base*		m_ebase;
			event*			m_time_reconnect_ebase;    // 服务器延迟重连定时器
			event*          m_keep_alive_ebase;		   // 应用层呼吸包
			bufferevent*    m_bufevserver; 			   // 数据缓冲
			std::string 	m_upstream_ip;
			uint32_t 		m_upstream_port;
			bool 			m_login_status;

			uint32_t 		m_dely_second;
			uint32_t 		m_max_dely_second;
		};
}

#endif /* SERVICE_DATACENTER_DOWNLOAD_BASE_SESSION_H_ */
