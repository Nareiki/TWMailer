#ifndef TWMAILER_SYNCMANAGER_H
#define TWMAILER_SYNCMANAGER_H


#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

class SyncManager {
public:
    static std::mutex& get_user_mutex(const std::string& username);

private:
    static std::unordered_map<std::string, std::mutex> user_mutexes_;
    static std::mutex map_mutex_;
};



#endif //TWMAILER_SYNCMANAGER_H