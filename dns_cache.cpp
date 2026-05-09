#include "dns_cache.h"
#include <iostream>
#include <fstream>
using namespace std;

DNSCache::DNSCache(int maxSize) : maxCacheSize(maxSize), cacheHits(0), cacheMisses(0) {
    cache = new HashTable<string>(maxSize);
}

DNSCache::~DNSCache() {
    delete cache;
}

bool DNSCache::get(const string& domain, DNSRecord& result) {
    string ip;
    bool found = cache->get(domain, ip);
    
    if (found) {
        // Create DNSRecord from cached IP
        result.domain = domain;
        result.type = A_RECORD;
        result.values.clear();
        result.values.insert(ip);
        result.ttl = 300;
        result.createdAt = time(nullptr);
        cacheHits++;
        return true;
    }
    
    cacheMisses++;
    return false;
}

bool DNSCache::get(const string& domain, string& ip) {
    bool found = cache->get(domain, ip);
    
    if (found) {
        cacheHits++;
        return true;
    }
    
    cacheMisses++;
    return false;
}

set<string>* DNSCache::getAll(const string& domain) {
    set<string>* ips = cache->getAll(domain);
    
    if (ips != nullptr && !ips->empty()) {
        cacheHits++;
        return ips;
    }
    
    cacheMisses++;
    return nullptr;
}

void DNSCache::put(const string& domain, const DNSRecord& record) {
    // Insert all IPs from DNSRecord into cache
    for (const auto& ip : record.values) {
        cache->insert(domain, ip);
    }
}

void DNSCache::put(const string& domain, const string& ip) {
    cache->insert(domain, ip);
}

void DNSCache::remove(const string& domain) {
    cache->remove(domain);
}

void DNSCache::addIP(const string& domain, const string& ip) {
    cache->insert(domain, ip);
}

void DNSCache::cleanExpired() {
    // For now, just a placeholder
    // In full implementation, would check TTL and remove expired entries
    cout << "Cleaning expired entries..." << endl;
}

int DNSCache::getCacheHits() const {
    return cacheHits;
}

int DNSCache::getCacheMisses() const {
    return cacheMisses;
}

float DNSCache::getHitRate() const {
    int total = cacheHits + cacheMisses;
    if (total == 0) return 0.0f;
    return (float)cacheHits / total * 100.0f;
}

int DNSCache::getCurrentSize() const {
    return cache->getKeyCount();
}

void DNSCache::clear() {
    cache->clear();
    cacheHits = 0;
    cacheMisses = 0;
}

void DNSCache::display() {
    cout << "\n========== DNS CACHE CONTENTS ==========" << endl;
    cache->displayChains();
    cout << "========================================\n" << endl;
}

void DNSCache::displayStats() {
    cout << "\n========== CACHE STATISTICS ==========" << endl;
    cout << "Cache Hits: " << cacheHits << endl;
    cout << "Cache Misses: " << cacheMisses << endl;
    cout << "Hit Rate: " << getHitRate() << "%" << endl;
    cout << "Current Size: " << getCurrentSize() << " domains" << endl;
    cout << "Total IPs: " << cache->getValueCount() << endl;
    cout << "Max Size: " << maxCacheSize << endl;
    cout << "Load Factor: " << cache->getLoadFactor() << endl;
    cout << "======================================\n" << endl;
}

void DNSCache::saveToFile(const string& filename) const {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << " for writing" << endl;
        return;
    }
    
    // Write cache metadata
    file.write(reinterpret_cast<const char*>(&cacheHits), sizeof(cacheHits));
    file.write(reinterpret_cast<const char*>(&cacheMisses), sizeof(cacheMisses));
    file.write(reinterpret_cast<const char*>(&maxCacheSize), sizeof(maxCacheSize));
    
    // Note: Full hash table serialization would go here
    // For now, just metadata
    
    file.close();
    cout << "Cache saved to " << filename << endl;
}

void DNSCache::loadFromFile(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Warning: Could not open file " << filename << " for reading" << endl;
        return;
    }
    
    // Read cache metadata
    file.read(reinterpret_cast<char*>(&cacheHits), sizeof(cacheHits));
    file.read(reinterpret_cast<char*>(&cacheMisses), sizeof(cacheMisses));
    file.read(reinterpret_cast<char*>(&maxCacheSize), sizeof(maxCacheSize));
    
    // Note: Full hash table deserialization would go here
    
    file.close();
    cout << "Cache loaded from " << filename << endl;
}