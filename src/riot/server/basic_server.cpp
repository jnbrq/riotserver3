#include <src/riot/server/basic_server.hpp>

namespace riot { namespace server {

basic_server::basic_server(io_service& io_service, short port) :
    server_common<
        async_stream_protocol<tcp::socket, basic_server>>(io_service),
    acceptor_(io_service, tcp::endpoint(ip::tcp::v4(), port)),
    socket_(io_service)
{
}

void basic_server::start()
{
    do_accept();
}

void basic_server::stop() {
    post([this] {
        acceptor_.cancel();
        for_each_session([this](auto session, bool remove) {
            session->async_stop();
            remove = true;
            return true;
        });
    });
}

void basic_server::do_accept()
{
    acceptor_.async_accept(socket_, [this](const error_code &err) {
        if (err) {
            // most probably boost::asio::error::operation_aborted
            return ;
        }
        auto protocol = std::make_shared<
                        async_stream_protocol<tcp::socket, basic_server>>(
            io_service_, std::move(socket_), *this);
        sessions.push_back(protocol);
        protocol->start();
        socket_ = tcp::socket(io_service_);
        do_accept();
    });
}

}}
