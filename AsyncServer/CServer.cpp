#include "CServer.h"


// 初始化服务器对象，绑定 I/O 上下文和监听的端口，并启动服务器
CServer::CServer(boost::asio::io_context& ioc, short port) : _ioc(ioc),
_acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {
	cout << "Server start success, on port: " << port << endl;
	// 开始异步地接受客户端连接请求。服务器启动后就进入等待客户端连接的状态
	start_accept();
}

void CServer::ClearSession(std::string uuid) {
	_sessions.erase(uuid);
}

void CServer::start_accept() {
	// make_shared分配并构造一个 std::shared_ptr,_ioc, this是传给Session的参数
	std::shared_ptr<CSession> new_session = std::make_shared<CSession>(_ioc, this);
	// 开始一个异步接受操作，当new_session的socket与客户端连接成功时，调用回调函数handle_accept
	// 为什么new_session在右括号结束后仍不结束，而是bind后计数加一？
	// new_session通过bind绑定时，new_session的计数就会加一，所以在bind后，new_session的生命周期和
	// 新构造函数的生命周期相同，因为新生成的函数对象引用了new_session（new_session通过值传递的方式被复制构造函数使用）。
	// 所以只要新构造的bind回调函数没有被调用、移除，new_session的声明周期就始终存在，所以new_session不会随着'}'的结束而释放。
	_acceptor.async_accept(new_session->Socket(), std::bind(&CServer::handle_accept, this, new_session,
		std::placeholders::_1));
}

// 当handle_accept触发时，也就是start_accept的回调函数被触发，当该回调函数结束后从队列中移除后，new_session的引用计数减一
void CServer::handle_accept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error) {
	// 如果没有错误（error 为 false），调用 new_session->Start() 来启动与旧客户端的会话
	if (!error) {
		new_session->Start();
		_sessions.insert(std::make_pair(new_session->GetUuid(), new_session));
	}
	else cout << "session accept failed, error is " << error.what() << endl;
	// 无论当前连接是否成功，都重新调用 start_accept()，以便服务器能够继续接受下一个新客户端的连接请求。
	// 服务器始终保持在监听状态，随时准备接受新连接
	start_accept();
}