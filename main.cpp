#include <list>
#include <thread>
#include <string>
#include <locale>

#include <src/riot/server/basic_server.hpp>
#include <src/riot/server/ssl_server.hpp>

int main(int argc, char **argv) {
#if 0
    using namespace riot::server;
    boost::asio::io_service io_service;
    boost::asio::io_service::work work(io_service); // ensure threads work
    basic_server server(io_service, 9990);
    std::list<std::thread> threads;
    for (int i = 0; i < 2; ++i) {
        threads.emplace_back([&io_service]() {
            io_service.run();
        });
    }
    server.start();
    io_service.run();
    return 0;
#endif
    
#if 1
    std::locale::global(std::locale("en_US.UTF-8"));
    using namespace riot::server;
    using namespace boost::asio;
    io_service io_serv;
    ssl::context sslctx(io_serv, ssl::context::sslv23);
    sslctx.set_options(
        ssl::context::default_workarounds |
        ssl::context::no_sslv2);
    sslctx.set_password_callback(
        [](std::size_t max_length, ssl::context::password_purpose purpose)
        -> std::string
        { return "qwerty112358"; });
    sslctx.use_certificate_file("../ssl/cert.pem", ssl::context::pem);
    sslctx.use_private_key_file("../ssl/key.pem", ssl::context::pem);
    ssl_server_standalone server(io_serv, sslctx, 9990);
    std::list<std::thread> threads;
    for (int i = 0; i < 3; ++i)
        threads.emplace_back([&io_serv, i]() {
            io_service::work work(io_serv);
            std::cout << "thread start: " + std::to_string(i) + "\n";
            io_serv.run();
            std::cout << "thread stop: " + std::to_string(i) + "\n";
        });
    server.start();
    io_service::work work(io_serv);
    io_serv.run();
    for (auto &t: threads) t.join();
    std::cout << "bye..." << std::endl;
    return 0;
#endif
}
