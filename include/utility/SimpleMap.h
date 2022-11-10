#pragma once

#include <map>
#include <unordered_map>
#include <string>
#include <iostream>

#include <utility/Exceptions.h>
#include <utility/Utility.h>

namespace saxs {
    namespace detail {
        /**
         * @brief A simple case-insensitive hashmap. 
         */
        template<typename V>
        struct SimpleMap {
            /**
             * @brief Create a new empty SimpleMap.
             */
            SimpleMap() {}

            /**
             * @brief Create a new SimpleMap from a std::map.
             */
            SimpleMap(std::unordered_map<std::string, V> map) {
                for (auto& [key, value] : map) {
                    insert(key, value);
                }
            }

            /**
             * @brief Get a value from the storage. 
             */
            virtual V get(std::string key) const {
                std::string k2 = utility::to_lowercase(key);
                if (data.find(k2) == data.end()) {
                    throw except::map_error("Error in SimpleMap::get: Key " + k2 + " not found in map");
                }
                return data.at(k2);
            }

            /**
             * @brief Insert a key-value pair into the storage. 
             */
            void insert(const std::string& key, V val) {
                std::string k2 = utility::to_lowercase(key);
                data.emplace(k2, val);
            }

            /**
             * @brief Check if this map contains the given key. 
             */
            bool contains(const std::string& key) const {
                return data.find(utility::to_lowercase(key)) != data.end();
            }

            typename std::unordered_map<std::string, V>::const_iterator begin() const {
                return data.begin();
            }

            typename std::unordered_map<std::string, V>::const_iterator end() const {
                return data.end();
            }

            typename std::unordered_map<std::string, V>::iterator begin() {
                return data.begin();
            }

            typename std::unordered_map<std::string, V>::iterator end() {
                return data.end();
            }

            std::unordered_map<std::string, V> data;
        };
    }
}