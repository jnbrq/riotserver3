#include <src/riot/server/ssl_server.hpp>

namespace riot { namespace server {

ssl_server_standalone::ssl_server_standalone(
    io_service &io_service,
    ssl::context &sslctx,
    unsigned short port) :
    server_common<async_stream_protocol<
        ssl::stream<ip::tcp::socket> & /* we can't use socket_type here */,
        ssl_server_standalone>>(io_service),
    sslctx_(sslctx),
    acceptor_(io_service_, tcp::endpoint(ip::tcp::v4(), port))
{
    
}

void ssl_server_standalone::start() {
    do_accept();
}

void ssl_server_standalone::stop() {
    post([this] {
        acceptor_.cancel();
        for_each_session([this](auto session, bool remove) {
            session->async_stop();
            remove = true;
            return true;
        });
    });
}

void ssl_server_standalone::do_accept() {
    connection_ = std::make_shared<connection>(*this);
    acceptor_.async_accept(
        connection_->lowest_layer(),
        [this](const error_code &err) {
            if (err) {
                // most probably boost::asio::error::operation_aborted
                return ;
            }
            connection_->stream().async_handshake(ssl::stream_base::server,
                wrap([this, connection = connection_](const error_code &ec) {
                    // this is wrapped by server only
                    if (ec) {
                        return ;
                    }
                    connection->start();   // no need for safety
                    sessions.push_back(connection);
            }));
            do_accept();
    });
}

ssl_server_standalone::connection::connection(ssl_server_standalone &server) :
    async_stream_protocol<
        ssl::stream<ip::tcp::socket> &,
        ssl_server_standalone>(server.io_service_, socket_, server),
    socket_(server.io_service_, server.sslctx_) {
    
}

}};
