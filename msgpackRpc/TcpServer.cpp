#pragma once
#include "TcpServer.h"

namespace msgpack {
namespace rpc {
namespace asio {

std::set<std::shared_ptr<TcpSession>> TcpServer::_sessions;

using boost::asio::io_service;
using boost::asio::ip::tcp;

TcpServer::TcpServer(io_service &ios, short port):
	_ioService(ios), _acceptor(ios, tcp::endpoint(tcp::v4(), port))
{
	_dispatcher = std::make_shared<msgpack::rpc::asio::dispatcher>();
} 

TcpServer::TcpServer(io_service &ios, const tcp::endpoint& endpoint):
	_ioService(ios),
	_acceptor(ios, endpoint)
{
	_dispatcher = std::make_shared<msgpack::rpc::asio::dispatcher>();
}

TcpServer::~TcpServer()
{
}

void TcpServer::start()
{
	startAccept();
}

void TcpServer::stop()
{
	_acceptor.close();
}

void TcpServer::startAccept()
{
	std::shared_ptr<TcpSession> pSession = std::make_shared<TcpSession>(_ioService);
	pSession->setDispatcher(_dispatcher);

	_acceptor.async_accept(pSession->getSocket(),
		[this, pSession](const boost::system::error_code& error)
	{
		if (!error)
		{
			_sessions.insert(pSession);
			pSession->start();
		}
		else
		{
			// log.error
		}

		startAccept();
	});
}

} } }