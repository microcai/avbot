#include "zmqpublisher.hpp"
#include "zmq.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/async_coro_queue.hpp>
#include <boost/thread.hpp>
#include <boost/circular_buffer.hpp>

class ZmqPublisher
{
public:
	struct Message {
		std::string channel_name;
		std::string body;
	};

	ZmqPublisher()
		: io_()
		, queue_(io_, 10)
	{
		ctx_ = zmq_ctx_new();
		socket_ = zmq_socket(ctx_, ZMQ_PUB);
		zmq_bind(socket_, "tcp://*:8123");
		queue_.async_pop(boost::bind(&ZmqPublisher::on_queue_pop, this, _1, _2));
		boost::thread(boost::bind(&asio::io_service::run, boost::ref(io_))).detach();
	}

	virtual ~ZmqPublisher()
	{
		queue_.cancele();
		zmq_close(socket_);
		zmq_ctx_term(ctx_);
	}

	void send(const std::string& channel, const std::string& str)
	{
		Message msg;
		msg.channel_name = channel;
		msg.body = str;
		queue_.push(msg);
	}

	void on_queue_pop(boost::system::error_code ec, const Message& value)
	{
		zmq_send(socket_, value.channel_name.c_str(), value.channel_name.size(), ZMQ_SNDMORE);
		zmq_send(socket_, value.body.c_str(), value.body.size(), 0);
	}

private:
	asio::io_service io_;
	boost::async_coro_queue< 
		boost::circular_buffer_space_optimized<
			Message
		> > queue_;
	void* ctx_;
	void* socket_;
};

class ZmqPublisherClient
{
public:
	ZmqPublisherClient(asio::io_service &io, std::string channel_name, boost::function<void (std::string)> sender)
		: io_(io)
		, channel_name_(channel_name)
		, sender_(sender)
	{
	}

	void operator()(boost::property_tree::ptree msg)
	{
		std::stringstream ss;
		boost::property_tree::json_parser::write_json(ss, msg);
		publisher_.send(channel_name_, ss.str());
	}

private:
	asio::io_service &io_;
	std::string channel_name_;
	boost::function<void (std::string)> sender_;

	static ZmqPublisher publisher_;
};

ZmqPublisher ZmqPublisherClient::publisher_;

avbot_extension make_zmq_publisher(asio::io_service &io, std::string channel_name, boost::function<void (std::string)> sender)
{
	return avbot_extension(channel_name, ZmqPublisherClient(io, channel_name, sender));
}