![Status](https://github.com/etbala/CacheDB/actions/workflows/cmake-single-platform.yml/badge.svg)

## Overview
CacheDB is an in-memory key-value store designed for high performance and low latency. It supports string keys and values, as well as sorted sets, with a command interface similar to that of Redis. CacheDB allows clients to perform common database operations such as setting and getting values, manipulating sorted sets, and managing key expirations (TTLs).

## Features

- In-Memory Storage: Fast data access with data stored directly in memory.
- String and Sorted Set (zset) Support: Manage simple key-value pairs and sorted sets for ordered data retrieval.
- TTL Management: Set expiration times for keys to automatically manage data lifecycle.
- Non-Blocking I/O: Handles multiple client connections efficiently.

## Getting Started

1. **Clone the repository:**
    ```bash
    git clone https://github.com/etbala/CacheDB.git
    cd CacheDB
    ```

2. **Build the project:**
    ```bash
    mkdir build
    cd build
    cmake ..
    make -j$(nproc)
    ```

3. **Run tests:**
    ```bash
    ./all_tests
    ```

## Commands

### GET

**Purpose**: Retrieves the string value associated with the specified key.

**Usage**: `get <key>`

**Behavior**: 

- If the key exists and is of type string, it returns the value.
- If the key doesn't exist or is of a different type, it returns `nil` or an error.

**Complexity**: O(1) average time.

### SET

**Purpose**: Stores a string value under the specified key.

**Usage**: `set <key> <value>`

**Behavior**: 

- If the key already exists and is of type string, it updates the value.
- If the key exists but is of a different type (e.g., a sorted set), it returns an error.
- If the key doesn't exist, it creates a new entry with the given key and value.

**Complexity**: O(1) average time.

### DEL

**Purpose**: Deletes the specified key from the database.

**Usage**: `del <key>`

**Behavior**: 

- Removes the key and its associated value from the database.
- Returns `1` if the key was removed, `0` if the key didn't exist.

**Complexity**: O(1) average time.

### KEYS

**Purpose**: Lists all the keys in the database.

**Usage**: `keys`

**Behavior**: 

- Returns an array of all keys stored in the database.

**Complexity**: O(N) time, where N is the number of keys in the database.

### ZADD

**Purpose**: Adds a member with a given score to a sorted set.

**Usage**: `zadd <zset_name> <score> <member>`

**Behavior**: 

- If the sorted set doesn't exist, it creates one.
- If the member already exists, it updates its score.

**Complexity**: O(log M), where M is the number of elements in the sorted set.

### ZREM

**Purpose**: Removes a member from a sorted set.

**Usage**: `zrem <zset_name> <member>`

**Behavior**: 

- If the member exists in the sorted set, it removes it and returns `1`.
- If the member doesn't exist, it returns `0`.

**Complexity**: O(log M), where M is the number of elements in the sorted set.

### ZSCORE

**Purpose**: Retrieves the score associated with a member in a sorted set.

**Usage**: `zscore <zset_name> <member>`

**Behavior**: 

- Returns the score if the member exists.
- Returns `nil` if the member doesn't exist.

**Complexity**: O(1) average time.

### ZQUERY

**Purpose**: Queries the sorted set for members starting from a given score and member.

**Usage**: `zquery <zset_name> <score> <member> <offset> <limit>`

**Behavior**: 

- Retrieves a list of members starting from the specified score and member.
- The offset and limit parameters control pagination.

**Complexity**: O(log M + R), where M is the number of elements in the sorted set and R is the number of results returned.

### PEXPIRE

**Purpose**: Sets a time-to-live (TTL) for a key in milliseconds.

**Usage**: `pexpire <key> <milliseconds>`

**Behavior**: 

- The key will expire and be automatically deleted after the specified time.
- If the key doesn't exist, it returns `0`.
- Returns `1` if the TTL was set successfully.

**Complexity**: O(log N), where N is the number of keys with a TTL set.

### PTTL

**Purpose**: Retrieves the remaining TTL for a key in milliseconds.

**Usage**: `pttl <key>`

**Behavior**: 

- Returns the remaining time in milliseconds before the key expires.
- Returns `-1` if the key exists but has no associated TTL.
- Returns `-2` if the key does not exist.

**Complexity**: O(1) average time.
