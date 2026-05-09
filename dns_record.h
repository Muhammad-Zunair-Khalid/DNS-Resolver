#ifndef DNS_RECORD_H
#define DNS_RECORD_H

#include <string>
#include <vector>
#include <set>
#include <ctime>
using namespace std;
// DNS Record Types
enum DNSRecordType {
    A_RECORD,      // IPv4 address
    AAAA_RECORD,   // IPv6 address
    CNAME_RECORD,  // Canonical name (alias)
    MX_RECORD,     // Mail exchange
    TXT_RECORD     // Text record
};

// DNS Record Structure
struct DNSRecord {
    std::string domain;              // Domain name (e.g., "google.com")
    DNSRecordType type;              // Record type
    std::set<std::string> values;    //  CHANGED: Multiple IPs (set instead of vector)
    int ttl;                         // Time to live (seconds)
    int priority;                    // Priority (for MX records)
    std::time_t createdAt;           // Creation timestamp

    // Constructors
    DNSRecord();
    DNSRecord(const std::string& domain, DNSRecordType type,
        const std::string& value, int ttl = 300);
    DNSRecord(const std::string& domain, DNSRecordType type,
        const std::set<std::string>& values, int ttl = 300);  // ✅ CHANGED: set parameter

    // Add value (for load balancing - multiple IPs)
    void addValue(const std::string& value);

    // Get single value (returns first from set)
    std::string getValue() const;

    // Get all values
    std::set<std::string> getAllValues() const;

    // Get value with round-robin (for load balancing)
    std::string getValueRoundRobin();

    // Check if value exists
    bool hasValue(const std::string& value) const;

    // Get number of values
    int getValueCount() const;

    // Serialization
    std::string serialize() const;
    static DNSRecord deserialize(const std::string& data);

    // Display
    std::string toString() const;
    static std::string typeToString(DNSRecordType type);
    static DNSRecordType stringToType(const std::string& typeStr);

private:
    static int roundRobinIndex;  // For load balancing
};

// Cache Entry (with TTL tracking)
struct CacheEntry {
    DNSRecord record;
    std::time_t cachedAt;        // When was it cached
    std::time_t expiresAt;       // When does it expire
    int hitCount;                // How many times accessed

    CacheEntry();
    CacheEntry(const DNSRecord& rec);

    // Check if expired
    bool isExpired() const;

    // Refresh expiry
    void refresh();

    // Serialization
    std::string serialize() const;
    static CacheEntry deserialize(const std::string& data);
};

#endif // DNS_RECORD_H