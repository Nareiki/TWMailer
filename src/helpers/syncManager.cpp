#include "helpers/syncManager.h"

/**
 * @file syncManager.cpp
 * @brief Implements user-specific locking for thread-safe access to per-user mailboxes.
 *
 * This module provides a shared map of mutexes, each assigned to a specific username.
 * It enables safe concurrent access to mail data (SEND, LIST, READ, DEL) on a per-user basis.
 */

// ====== Static Member Definitions ===========================================

// Stores a separate mutex for each username.
std::unordered_map<std::string, std::mutex> SyncManager::user_mutexes_;

// Mutex guarding access to the map itself.
std::mutex SyncManager::map_mutex_;

// ====== Public Methods ======================================================

/**
 * @brief Retrieves the mutex associated with a given user.
 *        If the mutex doesn't exist yet, it is created.
 * @param username The name of the user whose lock is requested.
 * @return A reference to the mutex associated with the user.
 *
 * @note Thread-safe: the map is guarded by a global mutex.
 * @warning Avoid long-held locks after acquiring a user mutex to prevent thread contention.
 */
std::mutex& SyncManager::get_user_mutex(const std::string& username) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    return user_mutexes_[username];
}
