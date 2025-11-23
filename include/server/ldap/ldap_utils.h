//
// Created by Christian Redl on 21.11.25.
//

#ifndef TW_MAILER_LDAP_UTILS_H
#define TW_MAILER_LDAP_UTILS_H
#include <string>


bool ldap_authenticate(const std::string&user, std::string&password);

#endif //TW_MAILER_LDAP_UTILS_H