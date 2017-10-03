#include "xeid_matcher.hpp"

#include <iostream>

namespace riot { namespace server {

xeid_matcher::xeid_matcher()
{
    do_cache();
}

xeid_matcher::xeid_matcher(const std::string& input)
{
    init(input);
}

xeid_matcher::xeid_matcher(xeid_matcher &&) = default;
xeid_matcher::xeid_matcher(const xeid_matcher&) = default;
xeid_matcher &xeid_matcher::operator=(xeid_matcher &&) = default;
xeid_matcher &xeid_matcher::operator=(const xeid_matcher &) = default;

void xeid_matcher::init(const std::string& input)
{
    std::regex re(R"(([^\s@#]+)(?:@([^\s@#]*)(?:#([^\s@#]*))?)?)");
    std::smatch m;
    std::regex_match(input, m, re);
    eid = m[1];
    dname = m[2];
    dtype = m[3];
    do_cache();
}

bool xeid_matcher::matches(const std::string& eid_str, const std::string& dname_str, const std::string& dtype_str)
{
    return
        (eid.empty() || std::regex_match(eid_str, reid_)) &&
        (dname.empty() || std::regex_match(dname_str, rdname_)) &&
        (dtype.empty() || std::regex_match(dtype_str, rdtype_));
}

bool xeid_matcher::device_matches(const std::string& dname_str, const std::string& dtype_str)
{
    return 
        (dname.empty() || std::regex_match(dname_str, rdname_)) &&
        (dtype.empty() || std::regex_match(dtype_str, rdtype_));
}

void xeid_matcher::do_cache()
{
    reid_ = std::regex(eid);
    rdname_ = std::regex(dname);
    rdtype_ = std::regex(dtype);
}

xeid_matcher & xeid_matcher::print()
{
    std::cout << *this << std::endl;
    return *this;
}

bool valid_identifier(const std::string &s)
{
    // yes, I was too lazy to not use regexes for this extremely simple thing
    static std::regex r(R"([a-zA-Z0-9_,-]*)");
    return std::regex_match(s, r);
}

}}

std::ostream & operator<<(std::ostream &os, const riot::server::xeid_matcher& xeidm)
{
    os << "eid: " << xeidm.eid << " dname: " << xeidm.dname << " dtype: " << xeidm.dtype;
    return os;
}
