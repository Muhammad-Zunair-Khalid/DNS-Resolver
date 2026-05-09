#include "stats_tracker.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <algorithm>

using namespace std;

// ==================== QueryStats Implementation ====================

QueryStats::QueryStats() {
    reset();
}

void QueryStats::reset() {
    totalQueries = 0;
    successfulQueries = 0;
    failedQueries = 0;
    cacheHits = 0;
    cacheMisses = 0;
    averageResponseTime = 0.0;
    totalResponseTime = 0.0;
}

string QueryStats::toString() const {
    stringstream ss;
    ss << "\n========== GLOBAL QUERY STATISTICS ==========\n";
    ss << "Total Queries:       " << totalQueries << "\n";
    ss << "Successful:          " << successfulQueries << "\n";
    ss << "Failed:              " << failedQueries << "\n";
    ss << "Cache Hits:          " << cacheHits << "\n";
    ss << "Cache Misses:        " << cacheMisses << "\n";

    float hitRate = 0.0f;
    if (totalQueries > 0) {
        hitRate = (float)cacheHits / totalQueries * 100.0f;
    }
    ss << "Cache Hit Rate:      " << fixed << setprecision(2) << hitRate << "%\n";

    ss << "Avg Response Time:   " << fixed << setprecision(2)
        << averageResponseTime << " ms\n";
    ss << "=============================================\n";

    return ss.str();
}

// ==================== DomainStat Implementation ====================

DomainStat::DomainStat() {
    domain = "";
    queryCount = 0;
    avgResponseTime = 0.0;
    lastQueried = time(nullptr);
}

DomainStat::DomainStat(const string& dom) {
    domain = dom;
    queryCount = 0;
    avgResponseTime = 0.0;
    lastQueried = time(nullptr);
}

void DomainStat::update(double responseTime) {
    queryCount++;

    // Update running average
    double total = avgResponseTime * (queryCount - 1) + responseTime;
    avgResponseTime = total / queryCount;

    lastQueried = time(nullptr);
}

// ==================== StatsTracker Implementation ====================

StatsTracker::StatsTracker() : statsMutex() {  // Initialize mutex first
    domainStats = new HashTable<DomainStat>(1024);
    globalStats.reset();
}
StatsTracker::~StatsTracker() {
    delete domainStats;
}

void StatsTracker::recordQuery(const string& domain, bool success,
    bool fromCache, double responseTime) {

    lock_guard<recursive_mutex> lock(statsMutex);

    // Update global stats
    globalStats.totalQueries++;

    if (success) {
        globalStats.successfulQueries++;
    }
    else {
        globalStats.failedQueries++;
    }

    if (fromCache) {
        globalStats.cacheHits++;
    }
    else {
        globalStats.cacheMisses++;
    }

    globalStats.totalResponseTime += responseTime;
    globalStats.averageResponseTime = globalStats.totalResponseTime / globalStats.totalQueries;

    // Update domain-specific stats
    DomainStat domainStat;
    if (domainStats->get(domain, domainStat)) {
        // Domain exists, update it
        domainStat.update(responseTime);
        domainStats->update(domain, domainStat);
    }
    else {
        // New domain
        DomainStat newStat(domain);
        newStat.update(responseTime);
        domainStats->insert(domain, newStat);
    }
}

QueryStats StatsTracker::getGlobalStats() {
    lock_guard<recursive_mutex> lock(statsMutex);
    return globalStats;
}

DomainStat StatsTracker::getDomainStats(const string& domain) {
    lock_guard<recursive_mutex> lock(statsMutex);

    DomainStat result;
    domainStats->get(domain, result);
    return result;
}

vector<DomainStat> StatsTracker::getTopDomains(int count) {
    lock_guard<recursive_mutex> lock(statsMutex);

    vector<DomainStat> allDomains;

    // ✅ ITERATE THROUGH HASH TABLE TO GET ALL DOMAINS
    // Access hash table's internal buckets
    int capacity = domainStats->getCapacity();

    for (int i = 0; i < capacity; i++) {
        // Get the chain at bucket i
        HashNode<DomainStat>* current = domainStats->table[i];

        while (current != nullptr) {
            // Get all DomainStat objects from the set
            for (const auto& stat : current->value) {
                allDomains.push_back(stat);
            }
            current = current->next;
        }
    }

    // Sort by query count (descending)
    sort(allDomains.begin(), allDomains.end(),
        [](const DomainStat& a, const DomainStat& b) {
            return a.queryCount > b.queryCount;
        });

    // Return top N
    if (allDomains.size() > (size_t)count) {
        allDomains.resize(count);
    }

    return allDomains;
}

void StatsTracker::displayGlobalStats() {
    lock_guard<recursive_mutex> lock(statsMutex);
    cout << globalStats.toString();
}

void StatsTracker::displayTopDomains(int count) {
    vector<DomainStat> topDomains = getTopDomains(count);

    cout << "\n========== TOP " << count << " QUERIED DOMAINS ==========\n";

    if (topDomains.empty()) {
        cout << "No domain statistics available yet.\n";
        cout << "================================================\n";
        return;
    }

    cout << left << setw(30) << "Domain"
        << setw(12) << "Queries"
        << setw(15) << "Avg Time (ms)"
        << "Last Queried\n";
    cout << string(70, '-') << "\n";

    for (const auto& stat : topDomains) {
        cout << left << setw(30) << stat.domain
            << setw(12) << stat.queryCount
            << setw(15) << fixed << setprecision(2) << stat.avgResponseTime
            << Utils::formatTimestamp(stat.lastQueried) << "\n";
    }

    cout << "================================================\n";
}

void StatsTracker::reset() {
    lock_guard<recursive_mutex> lock(statsMutex);

    globalStats.reset();
    domainStats->clear();
}

void StatsTracker::saveToFile(const string& filename) {
    lock_guard<recursive_mutex> lock(statsMutex);

    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << " for writing" << endl;
        return;
    }

    // Write global stats
    file.write(reinterpret_cast<const char*>(&globalStats.totalQueries), sizeof(int));
    file.write(reinterpret_cast<const char*>(&globalStats.successfulQueries), sizeof(int));
    file.write(reinterpret_cast<const char*>(&globalStats.failedQueries), sizeof(int));
    file.write(reinterpret_cast<const char*>(&globalStats.cacheHits), sizeof(int));
    file.write(reinterpret_cast<const char*>(&globalStats.cacheMisses), sizeof(int));
    file.write(reinterpret_cast<const char*>(&globalStats.averageResponseTime), sizeof(double));
    file.write(reinterpret_cast<const char*>(&globalStats.totalResponseTime), sizeof(double));

    file.close();
    cout << "Global statistics saved to " << filename << endl;

    //  Save domain stats hash table to separate file
    domainStats->saveToFile("domain_stats_hash.dat");
}

void StatsTracker::loadFromFile(const string& filename) {
    lock_guard<recursive_mutex> lock(statsMutex);

    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Warning: Could not open file " << filename << " for reading" << endl;
        return;
    }

    // Read global stats
    file.read(reinterpret_cast<char*>(&globalStats.totalQueries), sizeof(int));
    file.read(reinterpret_cast<char*>(&globalStats.successfulQueries), sizeof(int));
    file.read(reinterpret_cast<char*>(&globalStats.failedQueries), sizeof(int));
    file.read(reinterpret_cast<char*>(&globalStats.cacheHits), sizeof(int));
    file.read(reinterpret_cast<char*>(&globalStats.cacheMisses), sizeof(int));
    file.read(reinterpret_cast<char*>(&globalStats.averageResponseTime), sizeof(double));
    file.read(reinterpret_cast<char*>(&globalStats.totalResponseTime), sizeof(double));

    file.close();
    cout << "Global statistics loaded from " << filename << endl;

    //  Load domain stats hash table from separate file
    domainStats->loadFromFile("domain_stats_hash.dat");
}