#ifndef _xeid_matcher_included
#define _xeid_matcher_included

#include <string>
#include <regex>
#include <ostream>

namespace riot { namespace server {

class xeid_matcher {
public:
    std::string eid;
    std::string dname;
    std::string dtype;
public:
    xeid_matcher();
    xeid_matcher(const std::string &input);
    
    xeid_matcher(xeid_matcher &&);
    xeid_matcher(const xeid_matcher&);
    
    xeid_matcher &operator=(xeid_matcher &&);
    xeid_matcher &operator=(const xeid_matcher &);
    
    void init(const std::string &input);
    bool matches(
        const std::string &eid_str,
        const std::string &dname_str,
        const std::string &dtype_str
    );
    bool device_matches(
        const std::string &dname_str,
        const std::string &dtype_str
    );
    void do_cache();
    xeid_matcher &print();
private:
    std::regex reid_, rdname_, rdtype_;
};

}}

std::ostream &operator<<(std::ostream &, const riot::server::xeid_matcher &);

#endif // _xeid_matcher_included
