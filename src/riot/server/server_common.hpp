#ifndef _SERVER_COMMON_INCLUDED
#define _SERVER_COMMON_INCLUDED

#include <memory>
#include <list>
#include <boost/asio.hpp>

#include <src/riot/server/configuration.hpp>

namespace riot { namespace server {

using namespace boost::asio;

/* Protocol must provide wptr and ptr members */
/**
 * @brief server_common class should be inherited by the servers, or service
 * containers.
 * 
 */
template <typename Protocol>
class server_common :
    public std::enable_shared_from_this<server_common<Protocol>>,
    public strand {
public:
    /**
     * @brief constructor.
     * 
     * @param io_service io_service object on which the strand is constructed.
     */
    server_common(io_service &io_service) :
        strand(io_service),
        io_service_(io_service) {
    }
    
    /**
     * @brief list of the connections served by this server. connections should
     * be added only if they are successfully started (this is automatically
     * done if provided async_stream_protocol is used).
     * 
     */
    std::list<typename Protocol::wptr> sessions;
    
    server_configuration config;
    
    /**
     * @brief applies a callable to each session in this server. please not that
     * this function is not thread safe and it has to be called from a handler
     * wrapped with strand::wrap().
     * 
     * @param F callable type.
     * @param f callable object, forwarded.
     */
    template <typename F>
    void for_each_session(F &&f) {
        for (auto it = sessions.begin(); it != sessions.end();) {
            if (auto session = it->lock()) {
                bool remove = false;
                if (!f(session, remove)) {
                    break;
                }
                if (remove) {
                    auto old = it;
                    it++;
                    sessions.erase(old);
                }
                else
                    it++;
            }
            else {
                auto old = it;
                it++;
                sessions.erase(old);
            }
        }
    }
protected:
    io_service &io_service_;
};

}};

#endif // _SERVER_COMMON_INCLUDED
