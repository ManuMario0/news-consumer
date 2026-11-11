/*
* uid.hpp - Defines client UID.
*/

#pragma once

#include <stdexcept>
#include <string>

namespace nc {
    inline constexpr int MAX_CLIENTS = 32;

    /*
    * Controlled uid class to avoid out-of-bound accesses.
    */
    class UID {
        public:
            UID(const int uid): uid(uid) {
                if (uid >= MAX_CLIENTS || uid < 0) {
                    throw std::runtime_error(std::string("invalid UID provided"));
                }
            }

            int into_int() const {
                return uid;
            }
        
        private:
            const int uid;
    };
}
