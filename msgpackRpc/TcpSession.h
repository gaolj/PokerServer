#pragma once
#include "TcpConnection.h"
#include "Dispatcher.h"
#include <memory>	// enable_shared_from_this 
#include <boost/thread/future.hpp>

namespace msgpack {
namespace rpc {

const static int SESSION_TIMEOUT = 15;	// 15 second

typedef std::function<void(boost::shared_future<msgpack::object>& )> ResultCallback;
struct CallPromise
{
	boost::promise<msgpack::object> _prom;
	boost::shared_future<msgpack::object> _future;
	ResultCallback _callback;	// 内部通过future.get时可能会抛出异常，所以内部要用catch。（或调用它的地方要catch）

	CallPromise()
	{
		_future = _prom.get_future().share();
	}

	CallPromise(ResultCallback&& callback) : _callback(std::move(callback))
	{
		_future = _prom.get_future().share();
	}
};

class RequestFactory
{
public:
	template<typename... TArgs>
	MsgRequest<std::string, std::tuple<TArgs...>> create(const std::string& method, const TArgs... args);

private:
	uint32_t nextMsgid();
	uint32_t _nextMsgid = {1};
};

class TcpSession : public std::enable_shared_from_this<TcpSession>
{
public:
	TcpSession(boost::asio::io_service& ios, std::shared_ptr<Dispatcher> disp = nullptr);
	virtual ~TcpSession();

	void setDispatcher(std::shared_ptr<Dispatcher> disp);

	void init();
	void begin(boost::asio::ip::tcp::socket&& socket);
	void asyncConnect(const boost::asio::ip::tcp::endpoint& endpoint);

	void stop();
	void close();

	bool isConnected();
	void netErrorHandler(const boost::system::error_code& error, boost::exception_ptr pExcept);

	void waitforFinish();

	// Async call
	template<typename... TArgs>
	boost::shared_future<msgpack::object> call(const std::string& method, TArgs... args);

	template<typename... TArgs>
	void call(ResultCallback&& callback, const std::string& method, TArgs... args);

private:
	void processMsg(msgpack::unpacked upk, std::shared_ptr<TcpConnection> TcpConnection);

	boost::asio::io_service& _ioService;
	std::shared_ptr<TcpConnection> _connection;
	ConnectionHandler _connectionCallback;

	std::mutex _mutex;
	std::unordered_map<uint32_t, CallPromise> _mapRequest;
	RequestFactory _reqFactory;

	std::shared_ptr<Dispatcher> _dispatcher;
};

inline void TcpSession::waitforFinish()
{
	while (!_mapRequest.empty())
	{
		//局部锁，否则一wait_for，别的线程如果要lock，会死锁
		_mutex.lock();
		if (_mapRequest.empty())
			break;
		boost::shared_future<msgpack::object> fut = _mapRequest.begin()->second._future;
		if (fut.is_ready())
			_mapRequest.erase(_mapRequest.begin());
		_mutex.unlock();

		auto wt = fut.wait_for(boost::chrono::seconds(SESSION_TIMEOUT));
	}

	// 还要完善，加上没有完成的dispatcher处理的判断
}

// inline defination
template<typename... TArgs>
inline MsgRequest<std::string, std::tuple<TArgs...>> RequestFactory::create(const std::string& method, const TArgs... args)
{
	return MsgRequest<std::string, std::tuple<TArgs...>>(method, std::tuple<TArgs...>(args...), nextMsgid());
}

template<typename... TArgs>
inline boost::shared_future<msgpack::object> TcpSession::call(const std::string& method, TArgs... args)
{
	auto msgreq = _reqFactory.create(method, args...);
	auto sbuf = std::make_shared<msgpack::sbuffer>();
	msgpack::pack(*sbuf, msgreq);

	std::unique_lock<std::mutex> lck(_mutex);
	auto ret = _mapRequest.emplace(msgreq.msgid, CallPromise());
	_connection->asyncWrite(sbuf);

	return ret.first->second._future;
}

template<typename... TArgs>
inline void TcpSession::call(ResultCallback&& callback, const std::string& method, TArgs... args)
{
	auto msgreq = _reqFactory.create(method, args...);
	auto sbuf = std::make_shared<msgpack::sbuffer>();
	msgpack::pack(*sbuf, msgreq);

	std::unique_lock<std::mutex> lck(_mutex);
	_mapRequest.emplace(msgreq.msgid, CallPromise(std::move(callback)));
	_connection->asyncWrite(sbuf);
}

typedef std::shared_ptr<TcpSession> SessionPtr;

} }