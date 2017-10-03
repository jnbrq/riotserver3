#ifndef _HEADER_PARSER_INCLUDED
#define _HEADER_PARSER_INCLUDED

#include <string>
#include <map>
#include <sstream>
#include <cstdint>

namespace riot { namespace server {

class header_parser {
public:
    std::string version;
    std::string name;
    enum {
        normal = 0,
        uniquify = 1,
        enumerated = 2
    } name_flag { normal };
    std::string type;
    std::string password;
    enum {
        strong = 0,
        weak = 1
    } name_policy { strong };
    bool has_timeout {true};
    uint64_t timeout { static_cast<uint64_t>(1800e3) } /* in ms */;
    
    /**
     * @brief feeds line to the header parser. returns false if "end"
     * encountered, true otherwise.
     * 
     * @param line line to parse
     * @return bool
     */
    bool feed_line(const std::string &line);
    
    bool is_fine() const;
    std::string error_msg() const;
    
    static bool is_valid_version(const std::string &str);
    static bool is_valid_id(const std::string &str);
    static bool string_to_timeout(const std::string &str, bool &has_timeout, std::uint64_t &timeout);
private:
    int nline_ {0};
    std::string error_msg_;
    
    template <typename ...T>
    void set_error_msg(T && ...t) {
        std::ostringstream oss;
        oss << "syntax error (line = " << nline_ << "): ";
        using helper_t = int [];
        (void) helper_t { 0, ( oss << t, 0 ) ... };
        error_msg_ = oss.str();
    }
};

}}

#endif // _HEADER_PARSER_INCLUDED
