#pragma once
#ifndef HASH_TABLE_H
#define HASH_TABLE_H
#define HASH_SEED 5381
#include <string>
#include <set>
#include <cctype>
#include <vector>
#include <fstream>
#include <functional>
#include <type_traits>
#include <iostream>
#include <sstream>
using namespace std;

inline string toLLower(const string& s) {
    string res = s;
    for (char& c : res) c = tolower(c);
    return res;
}

template<typename ValueType>
struct HashNode {
    string key;
    set<ValueType> value;
    HashNode* next;

    HashNode(const string& k, const ValueType& v);
    ~HashNode();
};

// Hash Table with Chaining for DNS cache
template<typename ValueType>
class HashTable {
public:  // Made public for friend access in stats_tracker
    vector<HashNode<ValueType>*> table;   // Array of chain heads

private:
    int capacity;        // Bucket count (table size)
    int keyCount;        // Number of unique keys
    int valueCount;      // Total values across all keys
    int collisionCount;  // Number of collisions

    // Hash function
    unsigned int hash_domain(const string& key) const;
    unsigned int hashFunction(const string& key) const;

public:
    HashTable(int cap = 1024);
    ~HashTable();

    // Core operations
    void insert(const string& key, const ValueType& value);
    bool get(const string& key, ValueType& result);
    set<ValueType>* getAll(const string& key);
    bool remove(const string& key);
    bool exists(const string& key);

    // Update existing entry
    void update(const string& key, const ValueType& value);

    // Statistics
    int getValueCount() const;
    int getKeyCount() const;
    int getCapacity() const;
    float getLoadFactor() const;
    int getCollisionCount() const;

    // Display chains (for demonstration)
    void printStats() const;
    void displayChains();
    int getLongestChainLength() const;
    int getEmptyBuckets() const;

    // Clear
    void clear();

    // Get all entries for iteration
    void getAllEntries(vector<pair<string, set<ValueType>>>& entries);

    // Persistence - FULL SERIALIZATION
    void saveToFile(const string& filename);
    void loadFromFile(const string& filename);
};

// ==================== HashNode Implementation ====================

template <typename ValueType>
HashNode<ValueType>::HashNode(const string& k, const ValueType& v)
{
    key = k;
    value.insert(v);
    next = nullptr;
}

template <typename ValueType>
HashNode<ValueType>::~HashNode()
{
    key = "";
    value.clear();
    next = nullptr;
}

// ==================== HashTable Implementation ====================

template <typename ValueType>
unsigned int HashTable<ValueType>::hash_domain(const string& key) const
{
    unsigned int hash = HASH_SEED;
    for (char c : key) {
        if (c >= 'A' && c <= 'Z') {
            c = c | 0x20;
        }
        hash = ((hash << 5) + hash) + c;
    }
    return static_cast<int>(hash);
}

template <typename ValueType>
unsigned int HashTable<ValueType>::hashFunction(const string& key) const
{
    return hash_domain(key) % capacity;
}

template <typename ValueType>
HashTable<ValueType>::HashTable(int cap) : capacity(cap), keyCount(0), valueCount(0), collisionCount(0) {
    table.resize(cap, nullptr);
}

template <typename ValueType>
HashTable<ValueType>::~HashTable()
{
    for (int i = 0; i < capacity; i++) {
        HashNode<ValueType>* curr = table[i];
        while (curr != nullptr) {
            HashNode<ValueType>* temp = curr;
            curr = curr->next;
            temp->value.clear();
            delete temp;
        }
        table[i] = nullptr;
    }
}

template <typename ValueType>
void HashTable<ValueType>::insert(const string& key, const ValueType& value)
{
    int index = hashFunction(key);
    string LowerKey = toLLower(key);
    bool isNewBucket = (table[index] == nullptr);

    HashNode<ValueType>* current = table[index];
    while (current) {
        if (current->key == LowerKey) {
            auto result = current->value.insert(value);
            if (result.second) {
                valueCount++;
            }
            return;
        }
        current = current->next;
    }

    HashNode<ValueType>* newNode = new HashNode<ValueType>(LowerKey, value);
    newNode->next = table[index];
    table[index] = newNode;

    keyCount++;
    valueCount++;

    if (!isNewBucket) {
        collisionCount++;
    }
}

template <typename ValueType>
bool HashTable<ValueType>::get(const string& key, ValueType& result)
{
    int index = hashFunction(key);
    string LowerKey = toLLower(key);
    HashNode<ValueType>* curr = table[index];

    while (curr != nullptr) {
        if (curr->key == LowerKey) {
            if (!curr->value.empty()) {
                result = *(curr->value.begin());
                return true;
            }
            return false;
        }
        curr = curr->next;
    }
    return false;
}

template <typename ValueType>
set<ValueType>* HashTable<ValueType>::getAll(const string& key)
{
    int index = hashFunction(key);
    string LowerKey = toLLower(key);
    HashNode<ValueType>* curr = table[index];
    while (curr != nullptr) {
        if (curr->key == LowerKey) {
            return &(curr->value);
        }
        curr = curr->next;
    }
    return nullptr;
}

template <typename ValueType>
bool HashTable<ValueType>::remove(const string& key)
{
    int index = hashFunction(key);
    string LowerKey = toLLower(key);
    HashNode<ValueType>* current = table[index];
    HashNode<ValueType>* prev = nullptr;

    while (current) {
        if (current->key == LowerKey) {
            valueCount -= current->value.size();
            keyCount--;

            if (prev != nullptr || current->next != nullptr) {
                if (collisionCount > 0) {
                    collisionCount--;
                }
            }

            if (prev) {
                prev->next = current->next;
            }
            else {
                table[index] = current->next;
            }

            delete current;
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

template <typename ValueType>
bool HashTable<ValueType>::exists(const string& key)
{
    ValueType temp;
    string LowerKey = toLLower(key);
    return get(LowerKey, temp);
}

template <typename ValueType>
void HashTable<ValueType>::update(const string& key, const ValueType& value)
{
    int index = hashFunction(key);
    string LowerKey = toLLower(key);
    HashNode<ValueType>* curr = table[index];
    while (curr != nullptr) {
        if (curr->key == LowerKey) {
            int oldSize = curr->value.size();
            curr->value.clear();
            curr->value.insert(value);
            valueCount = valueCount - oldSize + 1;
            return;
        }
        curr = curr->next;
    }
}

template <typename ValueType>
int HashTable<ValueType>::getValueCount() const
{
    return valueCount;
}

template <typename ValueType>
int HashTable<ValueType>::getKeyCount() const
{
    return keyCount;
}

template <typename ValueType>
int HashTable<ValueType>::getCapacity() const
{
    return capacity;
}

template <typename ValueType>
float HashTable<ValueType>::getLoadFactor() const
{
    return (float)keyCount / capacity;
}

template <typename ValueType>
int HashTable<ValueType>::getCollisionCount() const
{
    return collisionCount;
}

template <typename ValueType>
int HashTable<ValueType>::getLongestChainLength() const
{
    int maxChainLength = 0;
    for (int i = 0; i < capacity; i++) {
        int chainLength = 0;
        HashNode<ValueType>* curr = table[i];
        while (curr != nullptr) {
            chainLength++;
            curr = curr->next;
        }
        if (chainLength > maxChainLength) {
            maxChainLength = chainLength;
        }
    }
    return maxChainLength;
}

template <typename ValueType>
int HashTable<ValueType>::getEmptyBuckets() const
{
    int emptyBuckets = 0;
    for (int i = 0; i < capacity; i++) {
        if (table[i] == nullptr) {
            emptyBuckets++;
        }
    }
    return emptyBuckets;
}

template <typename ValueType>
void HashTable<ValueType>::printStats() const
{
    cout << "Hash Table Stats:" << endl;
    cout << "  Capacity: " << getCapacity() << endl;
    cout << "  Total Items (IPs): " << getValueCount() << endl;
    cout << "  Total (key,value) pairs: " << getKeyCount() << endl;
    cout << "  Empty Buckets: " << getEmptyBuckets() << endl;
    cout << "  Max Chain Length: " << getLongestChainLength() << endl;
    cout << "  Load Factor: " << getLoadFactor() << endl;
    cout << "  Collisions: " << getCollisionCount() << endl;
}

template <typename ValueType>
void HashTable<ValueType>::displayChains()
{
    for (int i = 0; i < capacity; i++) {
        if (table[i] != nullptr) {
            HashNode<ValueType>* curr = table[i];
            while (curr != nullptr) {
                cout << "Bucket " << i << " - Key: " << curr->key << " | ";
                cout << "Value count: " << curr->value.size();

                if (curr->next != nullptr) {
                    cout << " -> ";
                }
                curr = curr->next;
            }
            cout << endl;
        }
    }
}

template <typename ValueType>
void HashTable<ValueType>::clear()
{
    for (int i = 0; i < capacity; i++) {
        HashNode<ValueType>* curr = table[i];
        while (curr != nullptr) {
            HashNode<ValueType>* temp = curr;
            curr = curr->next;
            delete temp;
        }
        table[i] = nullptr;
    }
    keyCount = 0;
    valueCount = 0;
    collisionCount = 0;
}

template <typename ValueType>
void HashTable<ValueType>::getAllEntries(vector<pair<string, set<ValueType>>>& entries) {
    entries.clear();
    for (int i = 0; i < capacity; i++) {
        HashNode<ValueType>* current = table[i];
        while (current != nullptr) {
            entries.push_back(make_pair(current->key, current->value));
            current = current->next;
        }
    }
}

// ==================== FULL SERIALIZATION (FOR STRING VALUES) ====================



template <typename ValueType>
void HashTable<ValueType>::saveToFile(const string& filename) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error: Could not save hash table to " << filename << endl;
        return;
    }

    // Write metadata only
    file.write(reinterpret_cast<const char*>(&capacity), sizeof(capacity));
    file.write(reinterpret_cast<const char*>(&keyCount), sizeof(keyCount));
    file.write(reinterpret_cast<const char*>(&valueCount), sizeof(valueCount));
    file.write(reinterpret_cast<const char*>(&collisionCount), sizeof(collisionCount));

    file.close();
    cout << "Hash table metadata saved to " << filename << endl;
}

template <typename ValueType>
void HashTable<ValueType>::loadFromFile(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Warning: Could not load hash table from " << filename << endl;
        return;
    }

    // Read metadata only
    file.read(reinterpret_cast<char*>(&capacity), sizeof(capacity));
    file.read(reinterpret_cast<char*>(&keyCount), sizeof(keyCount));
    file.read(reinterpret_cast<char*>(&valueCount), sizeof(valueCount));
    file.read(reinterpret_cast<char*>(&collisionCount), sizeof(collisionCount));

    table.resize(capacity, nullptr);

    file.close();
    cout << "Hash table metadata loaded from " << filename << endl;
}


#endif // HASH_TABLE_H



























