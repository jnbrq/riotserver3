#ifndef SSL_SERVER_INCLUDED
#define SSL_SERVER_INCLUDED

#include <src/riot/server/async_stream_protocol.hpp>
#include <src/riot/server/server_common.hpp>
#include <boost/asio/ssl.hpp>

namespace riot { namespace server {

using namespace boost::asio;
using namespace ip;

class ssl_server_standalone;

class ssl_server_standalone :
    public server_common<async_stream_protocol<
        ssl::stream<ip::tcp::socket> &,
        ssl_server_standalone>> {
public:
    ssl_server_standalone(
        io_service& io_service,
        ssl::context& sslctx, short unsigned int port);
    
    void start();
    
    void stop();
private:
    
    ssl::context &sslctx_;
    tcp::acceptor acceptor_;
    
    /* unfortunately, ssl::stream doesn't support move semantics */
    class connection :
        public async_stream_protocol<
            ssl::stream<ip::tcp::socket> &, ssl_server_standalone> {
    public:
        connection(ssl_server_standalone &server);
        using ptr = std::shared_ptr<connection>;
    private:
        ssl::stream<ip::tcp::socket> socket_;
    };
    
    friend class session;
    
    connection::ptr connection_;
    
    void do_accept();
};

}};

#endif // SSL_SERVER_INCLUDED
