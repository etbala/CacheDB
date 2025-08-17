#pragma once
#include <string>
#include "zset.h"

class Entry {
public:
    enum Type { STRING, ZSET };
    std::string key;
    Type type;
    std::string str_value;   // For STRING type
    ZSet* zset_value;        // For ZSET type

    Entry(const std::string& k, const std::string& val)
        : key(k), type(STRING), str_value(val), zset_value(nullptr) {}

    ~Entry() {
        if (type == ZSET && zset_value) {
            delete zset_value;
        }
    }
};
