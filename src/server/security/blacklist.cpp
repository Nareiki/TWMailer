//
// Created by Christian Redl on 23.11.25.
//

#include "security/blacklist.h"
#include <unordered_map>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>

struct BlacklistEntry {
    int failCount = 0;
    std::chrono::system_clock::time_point blockedUntil{};
    bool blocked = false;
};

static std::unordered_map<std::string, BlacklistEntry> g_blacklist;
static const int MAX_FAILS = 3;
static const std::chrono::seconds BLOCK_DURATION(60);
static std::string g_filename;

static std::chrono::system_clock::time_point tp_from_time_t(std::time_t t) {
    return std::chrono::system_clock::from_time_t(t);
}

static std::time_t time_t_from_tp(std::chrono::system_clock::time_point tp) {
    return std::chrono::system_clock::to_time_t(tp);
}

void load_blacklist(const std::string& filename) {
    g_filename = filename;
    g_blacklist.clear();

    std::ifstream in(filename);
    if (!in.is_open()) {
        return; // no file yet, that's fine
    }

    std::string line;
    auto now = std::chrono::system_clock::now();

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string ip;
        std::time_t until_ts;
        int failCount;
        bool blocked;
        if (!(iss >> ip >> until_ts >> failCount >> blocked)) {
            continue;
        }
        BlacklistEntry e;
        e.failCount = failCount;
        e.blockedUntil = tp_from_time_t(until_ts);
        e.blocked = blocked;
        // drop already expired entries
        if (e.blockedUntil > now) {
            g_blacklist[ip] = e;
        }
    }
}

void save_blacklist(const std::string& filename) {
    std::ofstream out(filename, std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "Warning: could not write blacklist file\n";
        return;
    }
    for (const auto& [ip, entry] : g_blacklist) {
        std::time_t until_ts = time_t_from_tp(entry.blockedUntil);
        out << ip << " " << until_ts << " " << entry.failCount << " " << entry.blocked << std::endl;
    }
}

bool is_ip_blocked(const std::string& ip) {
    auto it = g_blacklist.find(ip);
    if (it == g_blacklist.end()) return false;

    std::cout << it -> second.blocked << std::endl;

    if (!it->second.blocked) {
        return false;
    }

    auto now = std::chrono::system_clock::now();
    if (it->second.blocked && it->second.blockedUntil <= now) {
        // unblock expired
        g_blacklist.erase(it);
        save_blacklist(g_filename);
        return false;
    }
    return true;
}

void register_failed_login(const std::string& ip) {
    auto& entry = g_blacklist[ip];
    auto now = std::chrono::system_clock::now();

    std::cout << entry.blocked << std::endl;

    if (entry.blocked && entry.blockedUntil <= now) {
        entry.failCount = 0;
        entry.blockedUntil = {};
    }

    entry.failCount += 1;

    if (entry.failCount >= MAX_FAILS) {
        entry.blockedUntil = now + BLOCK_DURATION;
        entry.blocked = true;
        std::cerr << "IP " << ip << " blacklisted for 60s due to too many failed logins\n";
    }

    save_blacklist(g_filename);
}

void register_successful_login(const std::string& ip) {
    auto it = g_blacklist.find(ip);
    if (it != g_blacklist.end()) {
        g_blacklist.erase(it);
        save_blacklist(g_filename);
    }
}
