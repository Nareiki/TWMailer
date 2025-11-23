//
// Created by Christian Redl on 21.11.25.
//

#include "server/ldap/ldap_utils.h"

#include <string>

bool ldap_authenticate(const std::string& user, const std::string& password) {
    if (user.empty() || password.empty()) {
        return false;
    }
    return true;
}
