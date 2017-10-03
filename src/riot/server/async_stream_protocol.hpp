#ifndef SERVER_PROTOCOL_HPP_INCLUDED
#define SERVER_PROTOCOL_HPP_INCLUDED

#include <memory>
#include <iostream>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <regex>
#include <algorithm>
#include <utility>
#include <type_traits>
#include <thread>
#include <boost/asio.hpp>

#include <src/riot/server/header_parser.hpp>
#include <src/riot/server/command_parser.hpp>
#include <src/riot/server/xeid_matcher.hpp>

namespace riot { namespace server {

using namespace boost::asio;
using boost::system::error_code;


class async_stream_protocol_base :
    public std::enable_shared_from_this<async_stream_protocol_base>,
    public strand {
public:
    
    using ptr = std::shared_ptr<async_stream_protocol_base>;
    using wptr = std::weak_ptr<async_stream_protocol_base>;
    using buffer_type = std::vector<char>;
    using buffer_ptr_type = std::shared_ptr<buffer_type>;
    
    /**
     * @brief constructor.
     * 
     * @param io_service io_service object on which the strand is constructed.
     */
    async_stream_protocol_base(io_service &io_service) :
        strand(io_service),
        io_service_(io_service)
    {}
    
    /**
     * @brief posts a reading operation starting the connection.
     * this function has no thread safety protection, it is supposed to be
     * called only once, by the server.
     * 
     */
    virtual void start()
    {}
    
     /**
     * @brief posts a stop operation, cancelling everything on the stream
     * object.
     * 
     */
    virtual void async_stop()
    {}
    
    /**
     * @brief utility function converting any container object to
     * buffer_ptr_type.
     * 
     * @param T container type of char.
     * @param t container object of char.
     * @return async_stream_protocol_base::buffer_ptr_type same data converted
     * to buffer_ptr_type.
     */
    template <typename T>
    static buffer_ptr_type to_buffer(T &&t) {
        auto result = std::make_shared<buffer_type>(t.size());
        std::copy(t.begin(), t.end(), result->begin());
        return result;
    }
    
    /**
     * @brief posts a write operation to the strand.
     * 
     * @param buf data to write.
     */
    virtual void async_write(buffer_ptr_type buf)
    {}
    
    /**
     * @brief utility function for printing.
     * 
     */
    template <typename ...T>
    void async_print(T && ...t) {
        std::ostringstream oss;
        using helper_t = int [];
        (void) helper_t { 0, ( oss << t, 0 ) ... };
        async_write(to_buffer(oss.str()));
    }
    
    /**
     * @brief utility function for printing with a trailing new line.
     * 
     */
    template <typename ...T>
    void async_println(T && ...t) {
        async_print(t..., "\n");
    }
    
    /**
     * @brief posts a triggering operation to the device.
     * 
     * @param  trigging_device a shared pointer to the device triggering the
     * event on this device. its name() and type() must match with some of
     * the subscribed events, along with the eid of the trigger_xeidm.
     * @param  trigger_xeidm it's used for two purposes. The First purpose is 
     * that its eid member is checked in subscribed events. The second purpose
     * is checking if this device fits with the name and type conditions of the 
     * triggering device.
     * @param  data it's the triggering data.
     */
    virtual void async_trigger(
        ptr trigging_device,
        xeid_matcher trigger_xeidm,
        buffer_ptr_type data)
    {}
    
    /**
     * @brief returns the name of the device.
     * 
     * this function has no thread-safety protections, it must called only
     * after it's added to connection list, i.e. in phase_active.
     * 
     * @return std::string name of the device.
     */
    virtual std::string name() const
    {}
    
    /**
     * @brief returns a const reference to the request header.
     * 
     * @return const header_parser& request_header.
     */
    
    virtual const header_parser& header() const
    {}
    
    /**
     * @brief destructor.
     * 
     */
    virtual ~async_stream_protocol_base()
    {}
protected:
    io_service &io_service_;    
};

template <typename AsyncStream, typename Server>
class async_stream_protocol : public async_stream_protocol_base
{
public:
    /**
     * @brief constructor.
     * 
     * @param io_service io_service object to initialize the base.
     * @param s underlying async read.
     * @param server server object containing *this.
     */
    async_stream_protocol(
        io_service &io_service,
        AsyncStream &&s,
        Server &server) :
        async_stream_protocol_base(io_service),
        io_service_(io_service),
        server_(server),
        s_(std::forward<AsyncStream>(s)) {
    }
    
    /**
     * @brief returns a reference to the lowest layer provided by the
     * underlying stream.
     * 
     * @return auto& result of decay_t<AsyncStream>::lowest_layer()
     */
    auto &lowest_layer()
    { return s_.lowest_layer(); }
    
    /**
     * @brief returns a reference to the lowest layer provided by the
     * underlying stream.
     * 
     * @return const auto& result of decay_t<AsyncStream>::lowest_layer() const.
     */
    const auto &lowest_layer() const
    { return s_.lowest_layer(); }
    
    /**
     * @brief returns a reference to the underlying stream.
     * 
     * @return AsyncStream& underlying stream.
     */
    AsyncStream &stream()
    { return s_; }
    
    /**
     * @brief returns a const reference to the underlying stream.
     * 
     * @return AsyncStream& underlying stream.
     */
    const AsyncStream &stream() const
    { return s_; }
    
    /**
     * @brief returns a reference to the server to which this object belongs.
     * 
     * @return Server& owning server.
     */
    Server &server()
    { return server_; }
    
    /**
     * @brief returns a const reference to the server to which this object
     * belongs.
     * 
     * @return Server& owning server.
     */
    const Server &server() const
    { return server_; }
    
    /**
     * @brief posts a reading operation starting the connection.
     * this function has no thread safety protection, it is supposed to be
     * called only once, by the server.
     * 
     */
    void start() override {
        do_async_read();
    }
    
    /**
     * @brief posts a stop operation, cancelling everything on the stream
     * object.
     * 
     */
    void async_stop() override {
        // handlers must hold pointers to this shared pointer
        // to ensure it's alive
        post([this, c = this->shared_from_this()] {
            s_.lowest_layer().close();
        });
    }
    
    /**
     * @brief posts a write operation to the strand.
     * 
     * @param buf data to write.
     */
    void async_write(buffer_ptr_type buf) override {
        post([this, c = this->shared_from_this(), buf] {
            write_queue_.push_back(buf);
            if (write_queue_.size() == 1)   // no on-going write
                do_write();
        });
    }
    
    /**
     * @brief posts a triggering operation to the device.
     * 
     * @param  trigging_device a shared pointer to the device triggering the
     * event on this device. its name() and type() must match with some of
     * the subscribed events, along with the eid of the trigger_xeidm.
     * @param  trigger_xeidm it's used for two purposes. The First purpose is 
     * that its eid member is checked in subscribed events. The second purpose
     * is checking if this device fits with the name and type conditions of the 
     * triggering device.
     * @param  data it's the triggering data.
     */
    void async_trigger(
        ptr trigging_device,
        xeid_matcher trigger_xeidm,
        buffer_ptr_type data) override {
        // TODO a lot of code to come
    }
    
    /**
     * @brief returns the name of the device.
     * 
     * this function has no thread-safety protections, it must called only
     * after it's added to connection list, i.e. in phase_active.
     * 
     * @return std::string name of the device.
     */
    std::string name() const override {
        return name_;
    }
    
    /**
     * @brief returns a const reference to the request header.
     * 
     * @return const header_parser& request_header.
     */
    
    virtual const header_parser& header() const override {
        return header_;
    }
    
    /**
     * @brief destructor.
     * 
     */
    
    virtual ~async_stream_protocol() {
    }
    
private:
    
    io_service &io_service_;
    Server &server_;
    std::list<buffer_ptr_type> write_queue_;
    
    AsyncStream s_;
    streambuf streambuf_;
    
    enum phase_t : int {
        phase_newborn = 0,
        phase_intermediate,
        phase_active
    };
    phase_t phase_ { phase_newborn };
    
    header_parser header_;
    
    std::string name_;
    
    void do_async_read() {
        using namespace std::string_literals;
        async_read_until(s_, streambuf_, '\n', wrap(
            [this, c = this->shared_from_this()]
            (const error_code &ec, std::size_t /* bytes_transferred */) {
                // BEGIN error messages
                static const char *err_auth             = "authentication failed";
                // static const char *err_assign_name      = "cannot assing the name";
                static const char *err_multi_login      = "multiple login not allowed";
                static const char *err_not_init         = "argument not initialized";
                // END
                if (ec)
                    // most probably boost::asio::error::operation_aborted
                    return ;
                
                std::istream is(&streambuf_);
                std::string line;
                std::getline(is, line);
                
                switch (phase_)
                {
                case phase_newborn:
                {
                    if (!header_.feed_line(line))
                    {
                        /* we received an end message */
                        ((int&) phase_)++;  /* get into next phase */
                        if (!header_.is_fine()) {
                            async_println("ERROR ", header_.error_msg());
                            return ;
                        }
                        else {
                            /* no syntax error, check required args */
                            if (header_.name.empty()) {
                                async_println("ERROR ", err_not_init, " : name"s);
                                return ;
                            }
                            if (header_.type.empty()) {
                                async_println("ERROR ", err_not_init, " : type"s);
                                return ;
                            }
                            if (header_.version.empty()) {
                                async_println("ERROR ", err_not_init, " : RIOTp"s);
                                return ;
                            }
                            
                            server_.post([this, c /* keep ref */] {
                                /* check credentials */
                                bool multiple_login = false;
                                bool trusted = server_.config.check_credentials(
                                    header_.name,
                                    header_.password,
                                    multiple_login);
                                
                                if (!trusted) {
                                    async_println("ERROR ", err_auth);
                                    return ;
                                }
                                
                                /* search in valid names in server */
                                switch (header_.name_flag) {
                                case header_parser::normal: {
                                    bool name_free = true;
                                    server_.for_each_session([&](auto conn, bool &remove) -> bool {
                                        if (conn->name() == header_.name) {
                                            if (conn->header().name_policy == header_parser::weak) {
                                                conn->async_stop(); // stop the connection
                                                remove = true;
                                                // there cannot be other devices having this ID
                                                // so stop the loop
                                            }
                                            else {
                                                name_free = false;
                                            }
                                            return false;
                                        }
                                        return true;
                                    }) /* blocking, no need to keep ref */;
                                    if (name_free) {
                                        server_.sessions.push_back(c);
                                        ((int&) phase_)++;
                                        name_ = header_.name;
                                        async_println("OK ", name_);
                                        do_async_read();    // continue
                                        return ;
                                    }
                                    else {
                                        
                                        async_println("ERROR ", err_multi_login, ", not requested");
                                        return ;
                                    }
                                    break;
                                }
                                case header_parser::uniquify:   /* yes, they are the same thing, for now */
                                case header_parser::enumerated: {
                                    std::regex rgx {header_.name + "_(\\d+)"};
                                    std::list<uint64_t> occupied_numbers;
                                    server_.for_each_session([&](auto conn, bool &) -> bool {
                                        std::smatch m;
                                        auto name = conn->name();   // use of deleted function error?
                                        if (std::regex_match(name, m, rgx)) {
                                            occupied_numbers.push_back(std::stoul(m[1]));
                                        }
                                        return true;
                                    });
                                    if (occupied_numbers.empty()) {
                                        server_.sessions.push_back(c);
                                        ((int&) phase_)++;
                                        name_ = header_.name + "_1";
                                        async_println("OK ", name_);
                                        do_async_read();    // continue
                                        return ;
                                    }
                                    else {
                                        if (multiple_login) {
                                            int index = 1;
                                            while (std::find(occupied_numbers.begin(),
                                                            occupied_numbers.end(),
                                                            index) != occupied_numbers.end() /* it exists */)
                                                index++;
                                            /* now index is unique */
                                            server_.sessions.push_back(c);
                                            ((int&) phase_)++;
                                            name_ = header_.name + "_" + std::to_string(index);
                                            async_println("OK ", name_);
                                            do_async_read();    // continue
                                        }
                                        else {
                                            async_println("ERROR ", err_multi_login, ", administrator doesn't permit");
                                            return ;
                                        }
                                    }
                                    break;
                                }
                                }
                            });
                        }
                    }
                    else {
                        /* continue */
                        do_async_read();
                    }
                    break;
                }
                case phase_intermediate:
                {
                    // not possible
                    do_async_read();
                    break;
                }
                case phase_active:
                {
                    command_parser command;
                    if (command.parse(line)) {
                        switch (command.type()) {
                            case command_parser::trig: {
                                break;
                            }
                            case command_parser::sub: {
                                break;
                            }
                            case command_parser::negsub: {
                                break;
                            }
                            case command_parser::unnegsub: {
                                break;
                            }
                            case command_parser::pause: {
                                break;
                            }
                            case command_parser::cont: {
                                break;
                            }
                            case command_parser::p2p_accept: {
                                break;
                            }
                            case command_parser::p2p_stop_accept: {
                                break;
                            }
                            case command_parser::p2p_disconnect: {
                                break;
                            }
                            case command_parser::p2p_send: {
                                break;
                            }
                        }
                    }
                    else {
                        if (command.type() == command_parser::empty) {
                            /* empty line not an error */
                        }
                        else {
                            async_println("ERROR ", command.error_msg());
                        }
                    }
                    do_async_read();
                    break;
                }
                }
        }));
    }
    
    void do_write() {
        if (write_queue_.empty())
            return ;
        auto first = write_queue_.front();
        write_queue_.pop_front();
        boost::asio::async_write(s_, buffer(*first), wrap(
            [this, first /* must hold ! */, c = this->shared_from_this()](
                const error_code &ec,
                std::size_t bytes_transferred) {
                if (ec)
                    // most probably boost::asio::error::operation_aborted
                    return ;
                do_write();
        }));
    }
};



}};

#endif // SERVER_PROTOCOL_HPP_INCLUDED
