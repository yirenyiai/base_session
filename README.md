# base_session

base session 提供了一个libevent的封装, 通过 base_session 这个封装类, 你可以快速实现一个客户端对服务器的连接<br/>
在 base_session 内部实现了呼吸包, 自动重连机制.<br/>

base_decoder 提供了一个基础的解析器, 该解析器适用于 "固定协议头 + 包体" 系列的协议, 通过 base_decoder::set_deocer_cb 你可以设定一个回调, 你就可以在回调中处理对应的数据包.<br/>

net::util::get_body_size 是一个模板工具函数, 你可以通过重载包头的 uint32_t 操作, 这样子你就可以通过该函数直接从 固定协议头 中获取到对应的包体长度, 当然你可以通过模板的偏特化辅助你实现获取对应的包体长度.<br/>

# base_session 依赖
libevent, 更具体的信息你可以查看[libevent](http://libevent.org/ "libevent")的官方网站


# base_session 使用
针对他的使用, 我更建议你把他作为子模块进行使用, 在你的项目中自行添加依赖 libevent 的依赖.

