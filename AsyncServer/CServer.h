#pragma once
#include "CSession.h"

class CServer
{
private:
	void start_accept();  // 启动一个acceptor
	// 当acceptor接收到连接后启动该函数
	void handle_accept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error);
	boost::asio::io_context& _ioc;
	tcp::acceptor _acceptor;
	std::map<std::string, std::shared_ptr<CSession>> _sessions;
public:
	CServer(boost::asio::io_context& ioc, short port);
	void ClearSession(std::string uuid);
};
