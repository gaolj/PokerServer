#include <boost/test/included/unit_test.hpp>
#include "TcpConnection.h"
#include "TcpSession.h"
#include "TcpServer.h"
#include "SessionManager.h"
#include <iostream>
#include <boost/timer.hpp>

int count = 1;
const static int PORT = 8070;
using namespace msgpack::rpc;

int clientadd(int a, int b)
{
	std::cout << "client: handle add, " << a << " + " << b << std::endl;
	return a + b;
}

BOOST_AUTO_TEST_CASE(begin)
{
	boost::asio::io_service ios;
	Dispatcher disp;
	TcpConnection conn(ios);
	TcpSession ses(ios, nullptr);
	TcpServer server(ios, 8070);

	std::cout << "Dispatcher:" << sizeof(disp) << std::endl;
	std::cout << "TcpConnection:" << sizeof(ios) << std::endl;
	std::cout << "TcpSession:" << sizeof(ses) << std::endl;
	std::cout << "TcpServer:" << sizeof(server) << std::endl;

	BufferManager::instance();

	std::cout << "enter repeat times: ";
	std::cin >> count;
	std::cout << std::endl << std::endl;
}

//BOOST_AUTO_TEST_CASE(connect_close)
//{
//	std::cout << "BGN connect_close" << std::endl;
//	boost::asio::io_service client_io;
//	auto pWork = std::make_shared<boost::asio::io_service::work>(client_io);// *clinet_thread exit without work
//	boost::thread clinet_thread([&client_io]() { client_io.run(); });
//
//	{
//		boost::timer t;
//		for (int i = 0; i < count; i++)
//		{
//			auto session = std::make_shared<msgpack::rpc::TcpSession>(client_io, nullptr);
//			session->asyncConnect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), PORT));
//			session->close();	// session->_connection引用记数（asyncRead中加一的），会减一
//		}
//		std::cout << t.elapsed() << std::endl;
//	}
//
//	{
//		boost::timer t;
//		auto session = std::make_shared<msgpack::rpc::TcpSession>(client_io, nullptr);
//		for (int i = 0; i < count; i++)
//		{
//			session->asyncConnect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), PORT));
//			session->close();
//		}
//		std::cout << t.elapsed() << std::endl;
//	}
//
//	pWork.reset();
//	clinet_thread.join();
//	std::cout << "END connect_close" << std::endl << std::endl;
//}

//BOOST_AUTO_TEST_CASE(syncCall)
//{
//	std::cout << "BGN syncCall" << std::endl;
//
//	boost::asio::io_service client_io;
//	auto pWork = std::make_shared<boost::asio::io_service::work>(client_io);
//	boost::thread clinet_thread([&client_io]() { client_io.run(); });
//
//	try
//	{
//		boost::timer t;
//		for (int i = 0; i < count; i++)
//		{
//			auto session = std::make_shared<msgpack::rpc::TcpSession>(client_io, nullptr);
//			session->asyncConnect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), PORT));
//			auto fut = session->call("add", 1, 2).get();
//			BOOST_CHECK_EQUAL(fut.as<int>(), 3);
//			session->close();
//		}
//		/// ***局部变量session及成员_connection先析构，而weak.lock()后调用，所以在另一线程clinet_thread中处理由于close()的引起的async_read事件，
//		/// ***由于_connection._buf已经不存在，所以async_read会引发异常
//		std::cout << t.elapsed() << std::endl;
//	}
//	catch (const std::exception& e)
//	{
//		std::cerr << "call failed: " << e.what() << std::endl;
//	}
//	catch (...)
//	{
//		std::cerr << "call failed: " << std::endl;
//	}
//
//	try
//	{
//		auto session = std::make_shared<msgpack::rpc::TcpSession>(client_io, nullptr);
//		session->asyncConnect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), PORT));
//
//		boost::timer t;
//		for (int i = 0; i < count; i++)
//			BOOST_CHECK_EQUAL(session->call("add", 1, 2).get().as<int>(), 3);
//		std::cout << t.elapsed() << std::endl;
//		session->close();
//	}
//	catch (const std::exception& e)
//	{
//		std::cerr << "call failed: " << e.what() << std::endl;
//	}
//
//	pWork.reset();
//	clinet_thread.join();
//	std::cout << "END syncCall" << std::endl;
//}

BOOST_AUTO_TEST_CASE(ServerCallBack)
{
	std::cout << "BGN syncCall" << std::endl;

	boost::asio::io_service client_io;
	auto pWork = std::make_shared<boost::asio::io_service::work>(client_io);
	boost::thread clinet_thread([&client_io]() { client_io.run(); });

	//try
	//{
	//	// 有Dispatcher
	//	boost::timer t;
	//	for (int i = 0; i < count; i++)
	//	{
	//		std::shared_ptr<msgpack::rpc::Dispatcher> dispatcher = std::make_shared<msgpack::rpc::Dispatcher>();
	//		dispatcher->add_handler("add", &clientadd);

	//		auto session = std::make_shared<msgpack::rpc::TcpSession>(client_io, dispatcher);
	//		session->asyncConnect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), PORT));
	//		auto fut = session->call("mul", 1, 2).get();
	//		BOOST_CHECK_EQUAL(fut.as<int>(), 2);
	//		session->close();
	//	}
	//	std::cout << t.elapsed() << std::endl;
	//}
	//catch (const std::exception& e)
	//{
	//	std::cerr << "call failed: " << e.what() << std::endl;
	//}
	//catch (const msgpack::rpc::msgerror& e)
	//{
	//	std::cerr << "call failed: " << e.what() << std::endl;
	//}

	try
	{
		// 没有Dispatcher
		std::shared_ptr<msgpack::rpc::Dispatcher> dispatcher = std::make_shared<msgpack::rpc::Dispatcher>();
		dispatcher->add_handler("add", &clientadd);
		auto session = std::make_shared<msgpack::rpc::TcpSession>(client_io, dispatcher);
		session->asyncConnect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), PORT));

		boost::timer t;
		for (int i = 0; i < count; i++)
			BOOST_CHECK_EQUAL(session->call("mul", 1, 2).get().as<int>(), 2);
		std::cout << t.elapsed() << std::endl;
		session->close();

		// 有Dispatcher但没有handler

	}
	catch (const std::exception& e)
	{
		std::cerr << "call failed: " << e.what() << std::endl;
	}

	pWork.reset();
	clinet_thread.join();
	std::cout << "END syncCall" << std::endl;
}

BOOST_AUTO_TEST_CASE(end)
{
	std::cout << "enter something to exit test: ";
	std::string str;
	std::cin >> str;
}