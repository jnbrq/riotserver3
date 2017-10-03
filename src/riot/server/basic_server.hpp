#ifndef BASIC_SERVER_HPP_INCLUDED
#define BASIC_SERVER_HPP_INCLUDED

#include <mutex>
#include <list>

#include <src/riot/server/async_stream_protocol.hpp>
#include <src/riot/server/server_common.hpp>

namespace riot { namespace server {

using namespace boost::asio;
using namespace ip;

class basic_server :
    public server_common<
        async_stream_protocol<tcp::socket, basic_server>>
{

public:
    basic_server(
        io_service &io_service,
        short port);
    
    void start();
    
    void stop();
private:
    
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    
    void do_accept();
};

}}

#endif // BASIC_SERVER_HPP_INCLUDED
