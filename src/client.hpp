/*
* client.hpp - Defines client UID.
*/

#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

namespace nc {
    const static int MAX_CLIENTS = 32;

    /*
    * Controlled uid class to avoid out-of-bound accesses.
    */
    class UID {
        public:
            UID(int uid): uid(uid) {
                if (uid >= MAX_CLIENTS || uid < 0) {
                    throw std::runtime_error(std::string("invalid UID provided"));
                }
            }

            int into_int() {
                return uid;
            }
        
        private:
            int uid;
    };
}
