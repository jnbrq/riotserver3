#ifndef COMMAND_PARSER_INCLUDED
#define COMMAND_PARSER_INCLUDED

#include <list>
#include <cstdint>

#include <src/riot/server/xeid_matcher.hpp>

namespace riot { namespace server {

struct command_parser {
    enum type_t {
        empty = -2, // might help a little?
        invalid = -1,
        trig,
        sub,
        unsub,
        negsub,
        unnegsub,
        pause,
        cont,
        p2p_accept,
        p2p_stop_accept,
        p2p_disconnect,
        p2p_send
    };
    
    type_t type() const
    { return type_; }
    
    std::string error_msg() const
    { return error_msg_; }
    
    struct {
        struct {
            std::list<xeid_matcher> xeids;
        } trig;
        struct {
            std::list<xeid_matcher> xeids;
            bool minperiod_exists {false};
            std::uint64_t minperiod { static_cast<std::uint64_t>(1e6) } /* in ms */;
        } sub;
        struct {
            std::list<uint64_t> subIDs;
            bool all {false};
        } unsub;
        struct {
            std::list<xeid_matcher> xeids;
        } negsub;
        struct {
            std::list<uint64_t> negsubIDs;
            bool all {false};
        } unnegsub;
        struct {
            /* empty */
        } pause;
        struct {
            /* empty */
        } cont;
        struct {
            struct {
                bool maxconnections_exists { false };
                std::uint64_t maxconnections { 1000 };
            } accept;
            struct {
                /* empty */
            } stop_accept;
            struct {
                std::list<uint64_t> p2pIDs;
                bool all {false};
            } disconnect;
            struct {
                std::list<uint64_t> p2pIDs;
                std::uint64_t size {0};
                bool all {false};
                bool until_newline {false};
            } send;
        } p2p;
    } s;
    
    /**
     * @brief parses the given line, returns true if success, false otherwise
     * 
     * @param line line
     * @return bool
     */
    bool parse(const std::string &line);
private:
    type_t type_;
    std::string error_msg_;
 
    template <typename ...T>
    void set_error_msg(T && ...t) {
        std::ostringstream oss;
        oss << "syntax error: ";
        using helper_t = int [];
        (void) helper_t { 0, ( oss << t, 0 ) ... };
        error_msg_ = oss.str();
    }
};

}}

#endif // COMMAND_PARSER_INCLUDED
