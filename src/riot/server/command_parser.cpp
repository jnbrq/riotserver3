#include <sstream>
#include <stdexcept>
#include <utility>
#include <regex>

#include <src/riot/server/header_parser.hpp>
#include <src/riot/server/command_parser.hpp>

using namespace std;

namespace riot { namespace server {

bool command_parser::parse(const std::string &line) {
    // BEGIN error messages
    // static const char *err_not_enough_args      = "not enough arguments";
    static const char *err_too_many_args        = "too many arguments";
    // static const char *err_invalid_id           = "invalid identifier of p2pID/subID/negsubID";
    static const char *err_invalid_xeid         = "invalid xeid";
    static const char *err_invalid_arg          = "not a valid argument";
    static const char *err_invalid_command      = "not a valid command";
    static const char *err_emptyline            = "empty line";
    // END
    static regex rgx_p2p_send {R"((\d+(?:,\d+)*|\*)>(\d+|n|N))"};
    
    error_msg_ = "";
    istringstream iss(line);
    string dummy;
    if (iss >> dummy) {
        if (dummy == "trig") {
            type_ = trig;
            /* trig (<xeid>)*  */
            while (iss >> dummy) {
                xeid_matcher xeidm;
                try {
                    xeidm.init(dummy);
                    s.trig.xeids.push_back(std::move(xeidm));
                }
                catch (std::exception &ex) {
                    set_error_msg(err_invalid_xeid, " : ", ex.what());
                    break;
                }
            }
        }
        else if (dummy == "sub") {
            type_ = sub;
            /* reset first */
            s.sub.minperiod_exists = false;
            /* sub (<xeid>)* (minperiod=<timeout>)? */
            while (iss >> dummy) {
                xeid_matcher xeidm;
                try {
                    xeidm.init(dummy);
                    s.sub.xeids.push_back(std::move(xeidm));
                }
                catch (std::exception &ex) {
                    istringstream iss(dummy);
                    getline(iss, dummy, '=');
                    if (dummy == "minperiod") {
                        if (iss >> dummy /* iss has no whitespace, dummy is what is left */) {
                            if (header_parser::string_to_timeout(
                                dummy,
                                s.sub.minperiod_exists,
                                s.sub.minperiod)) {
                                /* fine */
                            }
                            else {
                                set_error_msg(err_invalid_arg, " : ", dummy);
                                break;
                            }
                        }
                        else {
                            set_error_msg(err_invalid_arg, " : ", dummy);
                            break;
                        }
                    }
                    else {
                        set_error_msg(err_invalid_arg, " : ", dummy);
                        break;
                    }
                }
            }
        }
        else if (dummy == "unsub") {
            type_ = unsub;
            /* reset first */
            s.unsub.all = false;
            /* unsub (subID)* \*? */
            while (iss >> dummy) {
                if (dummy == "*") {
                    s.unsub.all = true;
                    break;
                }
                else {
                    istringstream iss(dummy);
                    uint64_t subID;
                    if (iss >> subID) {
                        s.unsub.subIDs.push_back(subID);
                    }
                    else {
                        set_error_msg(err_invalid_arg, " : ", dummy);
                        break;
                    }
                }
            }
        }
        else if (dummy == "negsub") {
            type_ = negsub;
            /* negsub (<xeid>)* */
            while (iss >> dummy) {
                xeid_matcher xeidm;
                try {
                    xeidm.init(dummy);
                    s.negsub.xeids.push_back(std::move(xeidm));
                }
                catch (std::exception &ex) {
                    set_error_msg(err_invalid_xeid, " : ", ex.what());
                    break;
                }
            }
        }
        else if (dummy == "unnegsub") {
            type_ = unnegsub;
            /* reset first */
            s.unnegsub.all = false;
            /* unnegsub (subID)* \*? */
            while (iss >> dummy) {
                if (dummy == "*") {
                    s.unnegsub.all = true;
                    break;
                }
                else {
                    istringstream iss(dummy);
                    uint64_t negsubID;
                    if (iss >> negsubID) {
                        s.unnegsub.negsubIDs.push_back(negsubID);
                    }
                    else {
                        set_error_msg(err_invalid_arg, " : ", dummy);
                        break;
                    }
                }
            }
        }
        else if (dummy == "pause") {
            type_ = pause;
            /* pause */
        }
        else if (dummy == "continue") {
            type_ = cont;
            /* continue */
        }
        else if (dummy == "p2p-accept") {
            type_ = p2p_accept;
            /* reset first */
            s.p2p.accept.maxconnections_exists = false;
            /* p2p-accept (maxconnections=N)? */
            if (iss >> dummy) {
                istringstream iss(dummy);
                getline(iss, dummy, '=');
                if (dummy == "maxconnections") {
                    if (iss >> dummy) {
                        istringstream iss(dummy);
                        if (iss >> s.p2p.accept.maxconnections) {
                            s.p2p.accept.maxconnections_exists = true;
                        }
                        else {
                            set_error_msg(err_invalid_arg, " : ", dummy);
                        }
                    }
                    else {
                        set_error_msg(err_invalid_arg, " : ", dummy);
                    }
                }
                else {
                    set_error_msg(err_invalid_arg, " : ", dummy);
                }
            }
            else {
                /* argument is optional */
            }
        }
        else if (dummy == "p2p-stop-accept") {
            type_ = p2p_stop_accept;
            /* p2p-stop-accept */
        }
        else if (dummy == "p2p-disconnect") {
            type_ = p2p_disconnect;
            /* reset first */
            s.p2p.disconnect.all = false;
            /* p2p-disconnect (p2pID)* \*? */
            while (iss >> dummy) {
                if (dummy == "*") {
                    s.p2p.disconnect.all = true;
                    break;
                }
                else {
                    istringstream iss(dummy);
                    uint64_t p2pID;
                    if (iss >> p2pID) {
                        s.p2p.disconnect.p2pIDs.push_back(p2pID);
                    }
                    else {
                        set_error_msg(err_invalid_arg, " : ", dummy);
                    }
                }
            }
        }
        else {
            smatch match;
            if (regex_match(dummy, match, rgx_p2p_send)) {
                /* if not a command, can be send */
                /* reset first */
                s.p2p.send.all = false;
                s.p2p.send.until_newline = false;
                /* N1,N2,...,Nn>B */
                type_ = p2p_send;
                string m1 = match[1], m2 = match[2];
                if (m1 == "*")
                {
                    s.p2p.send.all = true;
                }
                else
                {
                    istringstream iss(m1);
                    uint64_t p2pID;
                    while (iss >> p2pID) {
                        s.p2p.send.p2pIDs.push_back(p2pID);
                        char c; iss >> c; // skip ","
                    }
                }
                
                if (m2 == "n" || m2 == "N") {
                    s.p2p.send.until_newline = true;
                }
                else {
                    istringstream iss(m2);
                    iss >> s.p2p.send.size;
                }
            }
            else {
                set_error_msg(err_invalid_command);
            }
        }
        
        /* if no error message is set, it's still possible to have too many arguments */
        if (error_msg_.empty() /* dont override previous error */ && iss >> dummy) {
            set_error_msg(err_too_many_args);
            type_ = invalid;
            return false;
        }
        
        /* if error message is set, return false */
        if (!error_msg_.empty()) {
            type_ = invalid;
            return false;
        }
        
        return true;
    }
    else {
        set_error_msg(err_emptyline);
        type_ = empty;
        return false;
    }
}

}}
