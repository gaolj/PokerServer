#pragma once
#include <memory>
#include <boost/asio.hpp>
#include "Dispatcher.h"

namespace msgpack {
namespace rpc {

class TcpServer
{
public:
	TcpServer(short port);
	TcpServer(const boost::asio::ip::tcp::endpoint& endpoint);
	virtual ~TcpServer();

	void start();
	void stop();

	void setDispatcher(std::shared_ptr<Dispatcher> disp);

private:
	void startAccept();

	boost::asio::io_service _ioService;
	boost::asio::ip::tcp::socket _socket;
	boost::asio::ip::tcp::acceptor _acceptor;
	std::shared_ptr<Dispatcher> _dispatcher;
	std::vector<std::thread> _threads;
};

} }