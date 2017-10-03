#include <sstream>
#include <regex>

#include <src/riot/server/header_parser.hpp>

using namespace std;

namespace riot { namespace server {

bool header_parser::is_valid_version(const string& str)
{
    static regex r(R"(\d+\.\d+)");
    return regex_match(str, r);
}

bool header_parser::is_valid_id(const string& str)
{
    static regex r(R"([a-zA-Z0-9_,-]+)");
    return regex_match(str, r);
}

bool header_parser::string_to_timeout(const string& str, bool& has_timeout, uint64_t& timeout)
{
    istringstream iss(str);
    long double dummy1;
    if (iss >> dummy1) {
        has_timeout = true;
        string dummy2;
        if (iss >> dummy2) {
            if (dummy2 == "s") {
                dummy1 *= 1e3;
            }
            else if (dummy2 == "ms") {
                dummy1 *= 1;
            }
            else if (dummy2 == "min") {
                dummy1 *= 60e3;
            }
            else if (dummy2 == "h") {
                dummy1 *= 3600e3;
            }
            else if (dummy2 == "day") {
                dummy1 *= 24 * 3600e3;
            }
            else if (dummy2 == "wk") {
                dummy1 *= 7 * 24 * 3600e3;
            }
            else {
                return false;
            }
            /* at this point we shouldn't have anything left in the stream */
            if (iss >> dummy2) {
                return false;
            }
            timeout = dummy1;
            return true;
        }
        else {
            /* default is millis */
            timeout = dummy1;
            return true;
        }
    }
    else {
        if (str == "inf") {
            has_timeout = false;
            return true;
        }
        else {
            return false;
        }
    }
}

bool header_parser::feed_line(const std::string& line)
{
    // BEGIN error messages
    static const char *err_riotp_appear_first   = "RIOTp must appear first";
    static const char *err_not_valid_version    = "not a valid version string";
    static const char *err_not_enough_args      = "not enough arguments";
    static const char *err_too_many_args        = "too many arguments";
    static const char *err_invalid_id           = "invalid identifier";
    static const char *err_invalid_arg          = "not a valid argument";
    static const char *err_invalid_command      = "not a valid command";
    // END
    ++nline_;
    istringstream iss(line);
    string dummy;
    if (iss >> dummy) {
        if (dummy == "END") {
            return false; // we are done!
        }
        else if (is_fine()) {
            if (dummy == "RIOTp") {
                if (nline_ == 1) {
                    if (iss >> dummy) {
                        if (is_valid_version(dummy)) {
                            version = dummy;
                        }
                        else {
                            set_error_msg(err_not_valid_version);
                        }
                    }
                    else {
                        set_error_msg(err_not_enough_args);
                    }
                }
                else {
                    set_error_msg(err_riotp_appear_first);
                }
            }
            /* end RIOTp */
            else if (dummy == "name:") {
                if (iss >> dummy) {
                    if (is_valid_id(dummy)) {
                        name = dummy;
                        if (iss >> dummy) {
                            if (dummy == "enumerated") {
                                name_flag = enumerated;
                            }
                            else if (dummy == "uniquify") {
                                name_flag = uniquify;
                            }
                            else {
                                set_error_msg(err_invalid_arg, " : ", dummy);
                            }
                        }
                        else {
                            name_flag = normal;
                        }
                    }
                    else {
                        set_error_msg(err_invalid_id);
                    }
                }
                else {
                    set_error_msg(err_not_enough_args);
                }
            }
            /* end name: */
            else if (dummy == "type:") {
                if (iss >> dummy) {
                    if (is_valid_id(dummy)) {
                        type = dummy;
                    }
                    else {
                        set_error_msg(err_invalid_id);
                    }
                }
                else {
                    set_error_msg(err_not_enough_args);
                }
            }
            /* end type: */
            else if (dummy == "password:") {
                if (iss >> dummy) {
                    password = dummy;
                }
                else {
                    set_error_msg(err_not_enough_args);
                }
            }
            /* end password: */
            else if (dummy == "name-policy:") {
                if (iss >> dummy) {
                    if (dummy == "weak") {
                        name_policy = weak;
                    }
                    else if (dummy == "strong") {
                        name_policy = strong;
                    }
                    else {
                        set_error_msg(err_invalid_arg, " : ", dummy);
                    }
                }
                else {
                    set_error_msg(err_not_enough_args);
                }
            }
            /* end name-policy: */
            else if (dummy == "timeout:") {
                if (iss >> dummy) {
                    if (string_to_timeout(dummy, has_timeout, timeout)) {
                    }
                    else {
                        set_error_msg(err_invalid_arg);
                    }
                }
                else {
                    set_error_msg(err_not_enough_args);
                }
            }
            /* end timeout: */
            else {
                set_error_msg(err_invalid_command);
            }
            
            /* ensure no more arguments are left */
            if (is_fine() && iss >> dummy) {
                // check is_fine() to not override previous message
                set_error_msg(err_too_many_args);
            }
            else {
            }
            return true;
        }
        else {
            return true;
        }
    }
    else {
        /* empty line is not an error */
        return true;
    }
}

bool header_parser::is_fine() const
{
    return error_msg_.empty();
}

std::string header_parser::error_msg() const
{
    return error_msg_;
}

}}
