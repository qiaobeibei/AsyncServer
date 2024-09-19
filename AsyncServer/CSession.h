#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <map>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <queue>

using boost::asio::ip::tcp;
using std::cout;
using std::cin;
using std::endl;

class CServer;

class MsgNode {
public:
	int _total_len; // 数据的总长度
	int _cur_len; // 已经处理的长度(已读的长度或者已写的长度)
	char* _msg; // 数据域首地址

	MsgNode(const char* msg, int total_len) :_total_len(total_len), _cur_len(0) { // 构造写节点
		_msg = new char[total_len];
		memcpy(_msg, msg, total_len);
	}
	MsgNode(int total_len) : _total_len(total_len), _cur_len(0) { // 构造读节点
		_msg = new char[total_len];
	}
	~MsgNode() {
		delete[] _msg;
	}
};

class CSession:public std::enable_shared_from_this<CSession>
{
private:
	tcp::socket _socket; // 处理客户端读写的套接字
	enum { max_length = 1024 };
	char _data[max_length]; 
	std::string _uuid;
	CServer* _server;
	std::queue<std::shared_ptr<MsgNode> > _send_que;
	std::mutex _send_lock;

	// headle回调函数
	void headle_read(const boost::system::error_code& error, size_t bytes_transferred,
		std::shared_ptr<CSession> _self_shared);
	void haddle_write(const boost::system::error_code& error, std::shared_ptr<CSession> _self_shared);

public:
	CSession(boost::asio::io_context& ioc, CServer* server) : _socket(ioc), _server(server){
		// random_generator是函数对象，加()就是函数，再加一个()就是调用该函数
		boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
		_uuid = boost::uuids::to_string(a_uuid);
	}
	tcp::socket& Socket() { return _socket; }
	const std::string& GetUuid() const { return _uuid; }
	void Start();
	void Send(char* msg, int max_length);
};



