#include "database_engine.h"
#include "utils.h"
#include <iostream>

using namespace std;

DatabaseEngine::DatabaseEngine(const string& path) : dbMutex() {
    dbPath = path;
    domainTree = new BTree<string, DNSRecord>(3);
    quickLookup = new HashTable<string>(1024);
    isInitialized = false;
}

DatabaseEngine::~DatabaseEngine() {
    flush();
    delete domainTree;
    delete quickLookup;
}

bool DatabaseEngine::initialize() {
    lock_guard<recursive_mutex> lock(dbMutex);

    if (isInitialized) {
        return true;
    }

    cout << "Initializing database at: " << dbPath << endl;

    // Try to load existing data
    load();

    isInitialized = true;
    cout << "Database initialized successfully" << endl;
    return true;
}

bool DatabaseEngine::exists() {
    return Utils::fileExists(dbPath + "/domains_btree.dat");
}

bool DatabaseEngine::addRecord(const DNSRecord& record) {
    lock_guard<recursive_mutex> lock(dbMutex);

    // Add to B-Tree
    domainTree->insert(record.domain, record);

    // Add to hash table (all IPs)
    for (const auto& ip : record.values) {
        quickLookup->insert(record.domain, ip);
    }

    return true;
}

bool DatabaseEngine::getRecord(const string& domain, DNSRecord& result) {
    lock_guard<recursive_mutex> lock(dbMutex);

    return domainTree->search(domain, result);
}

bool DatabaseEngine::getIP(const string& domain, string& ip) {
    lock_guard<recursive_mutex> lock(dbMutex);

    return quickLookup->get(domain, ip);
}

set<string>* DatabaseEngine::getAllIPs(const string& domain) {
    lock_guard<recursive_mutex> lock(dbMutex);

    return quickLookup->getAll(domain);
}

bool DatabaseEngine::addIP(const string& domain, const string& ip) {
    lock_guard<recursive_mutex> lock(dbMutex);

    // Check if record exists in B-tree
    DNSRecord record;
    if (domainTree->search(domain, record)) {
        // Add IP to existing record
        record.addValue(ip);
        domainTree->update(domain, record);
    }
    else {
        // Create new record
        record = DNSRecord(domain, A_RECORD, ip);
        domainTree->insert(domain, record);
    }

    // Add to hash table
    quickLookup->insert(domain, ip);

    return true;
}

bool DatabaseEngine::updateRecord(const DNSRecord& record) {
    lock_guard<recursive_mutex> lock(dbMutex);

    return domainTree->update(record.domain, record);
}

bool DatabaseEngine::deleteRecord(const string& domain) {
    lock_guard<recursive_mutex> lock(dbMutex);

    domainTree->remove(domain);
    quickLookup->remove(domain);

    return true;
}

bool DatabaseEngine::recordExists(const string& domain) {
    lock_guard<recursive_mutex> lock(dbMutex);

    DNSRecord temp;
    return domainTree->search(domain, temp);
}

vector<DNSRecord> DatabaseEngine::getSubdomains(const string& parentDomain) {
    lock_guard<recursive_mutex> lock(dbMutex);

    string startKey = parentDomain;
    string endKey = parentDomain + "~";

    return domainTree->rangeQuery(startKey, endKey);
}

vector<DNSRecord> DatabaseEngine::getAllRecords() {
    lock_guard<recursive_mutex> lock(dbMutex);

    vector<pair<string, DNSRecord>> allPairs = domainTree->getAllRecords();
    vector<DNSRecord> records;

    for (const auto& p : allPairs) {
        records.push_back(p.second);
    }

    return records;
}

bool DatabaseEngine::addRecords(const vector<DNSRecord>& records) {
    lock_guard<recursive_mutex> lock(dbMutex);

    for (const auto& record : records) {
        domainTree->insert(record.domain, record);

        for (const auto& ip : record.values) {
            quickLookup->insert(record.domain, ip);
        }
    }

    return true;
}

int DatabaseEngine::getTotalRecords() {
    lock_guard<recursive_mutex> lock(dbMutex);

    return domainTree->count();
}

int DatabaseEngine::getTotalIPs() {
    lock_guard<recursive_mutex> lock(dbMutex);

    return quickLookup->getValueCount();
}

void DatabaseEngine::displayStatistics() {
    lock_guard<recursive_mutex> lock(dbMutex);

    cout << "\n========== DATABASE STATISTICS ==========" << endl;
    cout << "Total DNS Records: " << domainTree->count() << endl;
    cout << "Total IP Addresses: " << quickLookup->getValueCount() << endl;
    cout << "B-Tree Height: " << domainTree->getHeight() << endl;
    cout << "Hash Table Load Factor: " << quickLookup->getLoadFactor() << endl;
    cout << "Hash Table Collisions: " << quickLookup->getCollisionCount() << endl;
    cout << "=========================================\n" << endl;
}

int DatabaseEngine::getBTreeHeight() {
    lock_guard<recursive_mutex> lock(dbMutex);

    return domainTree->getHeight();
}

int DatabaseEngine::getHashTableCollisions() {
    lock_guard<recursive_mutex> lock(dbMutex);

    return quickLookup->getCollisionCount();
}

bool DatabaseEngine::flush() {
    lock_guard<recursive_mutex> lock(dbMutex);

    cout << "Flushing database to disk..." << endl;

    domainTree->saveToFile(dbPath + "/domains_btree.dat");
    quickLookup->saveToFile(dbPath + "/quicklookup_hash.dat");

    cout << "Database flushed successfully" << endl;
    return true;
}

bool DatabaseEngine::load() {
    lock_guard<recursive_mutex> lock(dbMutex);

    cout << "Loading database from disk..." << endl;

    if (Utils::fileExists(dbPath + "/domains_btree.dat")) {
        domainTree->loadFromFile(dbPath + "/domains_btree.dat");
    }

    if (Utils::fileExists(dbPath + "/quicklookup_hash.dat")) {
        quickLookup->loadFromFile(dbPath + "/quicklookup_hash.dat");
    }

    cout << "Database loaded successfully" << endl;
    return true;
}

bool DatabaseEngine::backup(const string& backupPath) {
    lock_guard<recursive_mutex> lock(dbMutex);

    cout << "Creating backup at: " << backupPath << endl;

    domainTree->saveToFile(backupPath + "/domains_btree_backup.dat");
    quickLookup->saveToFile(backupPath + "/quicklookup_hash_backup.dat");

    cout << "Backup created successfully" << endl;
    return true;
}

bool DatabaseEngine::restore(const string& backupPath) {
    lock_guard<recursive_mutex> lock(dbMutex);

    cout << "Restoring from backup: " << backupPath << endl;

    domainTree->loadFromFile(backupPath + "/domains_btree_backup.dat");
    quickLookup->loadFromFile(backupPath + "/quicklookup_hash_backup.dat");

    cout << "Restore completed successfully" << endl;
    return true;
}