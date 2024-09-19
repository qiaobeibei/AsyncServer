#include "CSession.h"
#include "CServer.h"

void CSession::Start() {
	memset(_data, 0, max_length); // 缓冲区清零
	// 从套接字中读取数据，并绑定回调函数headle_read
	_socket.async_read_some(boost::asio::buffer(_data, max_length),
		// 这里可以将shared_ptr<Session>(this)给bind绑定吗？
		// 不可以，会造成多个智能指针绑定同一块内存的问题
		std::bind(&CSession::headle_read, this, std::placeholders::_1, std::placeholders::_2,
			shared_from_this()));
}

void CSession::headle_read(const boost::system::error_code& error, size_t bytes_transferred,
	std::shared_ptr<CSession> _self_shared) {
	if (!error) {
		cout << "server receive data is " << _data << endl;
		Send(_data, bytes_transferred); // 将收到的消息回传
		memset(_data, 0, max_length); // 缓冲区清零
		_socket.async_read_some(boost::asio::buffer(_data, max_length), std::bind(&CSession::headle_read, this,
			std::placeholders::_1, std::placeholders::_2, _self_shared));
	}
	else {
		std::cout << "handle read failed, error is " << error.what() << endl;
		_server->ClearSession(_uuid);
	}
}

// 异步写操作完成后的回调处理函数
void CSession::haddle_write(const boost::system::error_code& error, std::shared_ptr<CSession> _self_shared) {
	if (!error) { // 检查异步写是否成功
		std::lock_guard<std::mutex> lock(_send_lock); // 加锁保护发送队列
		_send_que.pop(); // 移除上一个已发送的消息（send函数中的异步发）
		if (!_send_que.empty()) { // 若队列不为空，处理下一个消息
			auto& msgnode = _send_que.front();
			boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_msg, msgnode->_total_len),
				std::bind(&CSession::haddle_write, this, std::placeholders::_1, _self_shared));
		}
	}
	else {
		std::cout << "handle write failed, error is " << error.what() << endl;
		_server->ClearSession(_uuid);
	}
}

void CSession::Send(char* msg, int max_length) {
	bool pending = false; // 发送标志，true时有未完成的发送操作，false为空
	// 使用lock_guard锁住_send_lock，确保_send_lock（发送队列）访问的线程安全的
	// 锁的存在确保了多个线程不会同时修改发送队列
	std::lock_guard<std::mutex> lock(_send_lock);
	// 判断队列是否有未完成的发送操作
	if (_send_que.size() > 0) {
		pending = true;
	}
	_send_que.push(std::make_shared<MsgNode>(msg, max_length)); // 将发送消息存储至队列
	if (pending) { // 如果有未完成的发送，直接返回
		return;
	}
	// 异步发送
	boost::asio::async_write(_socket, boost::asio::buffer(msg, max_length),
		std::bind(&CSession::haddle_write, this, std::placeholders::_1, shared_from_this()));
} // 当'}'结束后，_send_lock解锁，发送队列解锁
