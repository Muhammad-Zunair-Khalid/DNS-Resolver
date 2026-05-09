#ifndef DNS_CACHE_H
#define DNS_CACHE_H

#include "hash_table.h"
#include "dns_record.h"
#include <string>
#include <set>
using namespace std;
// DNS Cache with TTL management
class DNSCache {
private:
    HashTable<string>* cache;  
    int cacheHits;
    int cacheMisses;
    int maxCacheSize;

public:
    DNSCache(int maxSize = 1000);
    ~DNSCache();

    //  MODIFIED: Cache operations work with sets
    bool get(const string& domain, DNSRecord& result);
    bool get(const string& domain, string& ip);  // Get single IP
    std::set<std::string>* getAll(const string& domain);  // NEW: Get all IPs

    void put(const string& domain, const DNSRecord& record);
    void put(const string& domain, const string& ip);  // Add single IP

    void remove(const string& domain);

    // NEW: Add IP to existing domain
    void addIP(const string& domain, const string& ip);

    // Clean expired entries
    void cleanExpired();

    // Statistics
    int getCacheHits() const;
    int getCacheMisses() const;
    float getHitRate() const;
    int getCurrentSize() const;

    // Clear cache
    void clear();

    // Display cache contents
    void display();

    //  NEW: Display statistics
    void displayStats();

    // Persistence
    void saveToFile(const std::string& filename) const;
    void loadFromFile(const std::string& filename);
};

#endif // DNS_CACHE_H