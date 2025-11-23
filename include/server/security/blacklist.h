//
// Created by Christian Redl on 23.11.25.
//

#ifndef TW_MAILER_BLACKLIST_H
#define TW_MAILER_BLACKLIST_H
#include <string>

// Call once on server startup
void load_blacklist(const std::string& filename);

// Call whenever blacklist changes (or periodically)
void save_blacklist(const std::string& filename);

// Check if IP is currently blocked (and clean up expired entries)
bool is_ip_blocked(const std::string& ip);

// Call on failed login attempt
void register_failed_login(const std::string& ip);

// Call on successful login
void register_successful_login(const std::string& ip);

#endif //TW_MAILER_BLACKLIST_H