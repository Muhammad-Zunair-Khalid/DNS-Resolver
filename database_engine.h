#ifndef DATABASE_ENGINE_H
#define DATABASE_ENGINE_H
using namespace std;
#include "btree.h"
#include "hash_table.h"
#include "dns_record.h"
#include <string>
#include <set>
#include <mutex>

// Custom Database Engine for DNS Server
class DatabaseEngine {
private:
    std::string dbPath;

    // B-Tree for hierarchical domain storage
    BTree<std::string, DNSRecord>* domainTree;

    //CHANGED: Hash table stores set<string> for multiple IPs
    HashTable<string>* quickLookup;

    recursive_mutex dbMutex;  // Thread safety
    bool isInitialized;

public:
    DatabaseEngine(const std::string& path = "./dns_db");
    ~DatabaseEngine();

    // Initialize database
    bool initialize();
    bool exists();

    //  MODIFIED: DNS Record operations work with sets
    bool addRecord(const DNSRecord& record);
    bool getRecord(const std::string& domain, DNSRecord& result);

    //  NEW: Get single IP or all IPs
    bool getIP(const std::string& domain, std::string& ip);
    std::set<std::string>* getAllIPs(const std::string& domain);

    //  NEW: Add IP to existing domain
    bool addIP(const std::string& domain, const std::string& ip);

    bool updateRecord(const DNSRecord& record);
    bool deleteRecord(const std::string& domain);
    bool recordExists(const std::string& domain);

    // Query operations
    std::vector<DNSRecord> getSubdomains(const std::string& parentDomain);
    std::vector<DNSRecord> getAllRecords();

    // Bulk operations
    bool addRecords(const std::vector<DNSRecord>& records);
    int getTotalRecords();

    //NEW: Get total IPs across all domains
    int getTotalIPs();

    // Statistics
    void displayStatistics();
    int getBTreeHeight();
    int getHashTableCollisions();

    // Persistence
    bool flush();  // Write to disk
    bool load();   // Load from disk

    // Backup
    bool backup(const std::string& backupPath);
    bool restore(const std::string& backupPath);
};

#endif // DATABASE_ENGINE_H