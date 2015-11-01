#include "TcpSession.h"
#include <functional>	// std::bind
#include "SessionManager.h"

namespace msgpack {
namespace rpc {

using boost::asio::ip::tcp;
using std::placeholders::_1;
using std::placeholders::_2;

uint32_t RequestFactory::nextMsgid()
{
	return _nextMsgid++;
}

TcpSession::TcpSession(boost::asio::io_service& ios, std::shared_ptr<Dispatcher> disp):
	_ioService(ios),
	_dispatcher(disp)
{
}

void TcpSession::setDispatcher(std::shared_ptr<Dispatcher> disp)
{
	_dispatcher = disp;
}

void TcpSession::begin(tcp::socket socket)
{
	_connection = std::make_shared<TcpConnection>(std::move(socket));

	_connection->setMsgHandler(std::bind(&TcpSession::processMsg, shared_from_this(), _1, _2));
	_connection->setNetErrorHandler(std::bind(&TcpSession::netErrorHandler, shared_from_this(), _1));
	_connection->setConnectionHandler(_connectionCallback);

	_connection->startRead();
}

void TcpSession::asyncConnect(const boost::asio::ip::tcp::endpoint& endpoint)
{
	_connection = std::make_shared<TcpConnection>(_ioService);

	_connection->setMsgHandler(std::bind(&TcpSession::processMsg, shared_from_this(), _1, _2));
	_connection->setNetErrorHandler(std::bind(&TcpSession::netErrorHandler, shared_from_this(), _1));
	_connection->setConnectionHandler(_connectionCallback);

	_connection->asyncConnect(endpoint);

}

void TcpSession::stop()
{
	_connection.reset();
}

void TcpSession::close()
{
	_connection->close();
}

bool TcpSession::isConnected()
{
	return _connection->getConnectionStatus() == connection_connected;
}

void TcpSession::netErrorHandler(boost::system::error_code & error)
{
	SessionManager::instance()->stop(shared_from_this());
}

void TcpSession::processMsg(const object &msg, std::shared_ptr<TcpConnection> TcpConnection)
{
	MsgRpc rpc;
	msg.convert(&rpc);
	switch (rpc.type) {
	case MSG_TYPE_REQUEST:
		_dispatcher->dispatch(msg, TcpConnection);
		break;

	case MSG_TYPE_RESPONSE:
	{
		MsgResponse<object, object> res;
		msg.convert(&res);
		auto found = _mapRequest.find(res.msgid);
		if (found == _mapRequest.end())
		{
			throw client_error("no request for response");
		}
		else
		{
			auto prom = found->second;
			_mapRequest.erase(found);
			if (res.error.type == msgpack::type::NIL)
			{
				prom->set_value(res.result);
			}
			else if (res.error.type == msgpack::type::BOOLEAN)
			{
				bool isError;
				res.error.convert(&isError);
				if (isError)
					// http://stackoverflow.com/questions/22010388/converting-stdexception-ptr-to-boostexception-ptr
					prom->set_exception(boost::copy_exception(std::runtime_error("some error")));
				else
					prom->set_value(res.result);
			}
		}
	}
	break;

	case MSG_TYPE_NOTIFY:
	{
		MsgNotify<object, object> req;
		msg.convert(&req);
	}
	break;

	default:
		throw client_error("rpc type error");
	}
}

} }