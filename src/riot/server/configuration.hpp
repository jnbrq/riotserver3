#ifndef _CONFIGURATION_INCLUDED
#define _CONFIGURATION_INCLUDED

#include <string>

namespace riot { namespace server {

class server_configuration {

public:

    bool check_credentials(
        const std::string &name,
        const std::string &password,
        bool &multiple_login_allowed)
    {
        // temporarily
        multiple_login_allowed = true;
        return true;
    }

};

}};

#endif // _CONFIGURATION_INCLUDED
