#pragma once
#ifndef STATS_TRACKER_H
#define STATS_TRACKER_H

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <sstream>
#include <iomanip>
#include "hash_table.h"

using namespace std;

// Query Statistics
struct QueryStats {
    int totalQueries;
    int successfulQueries;
    int failedQueries;
    int cacheHits;
    int cacheMisses;
    double averageResponseTime;
    double totalResponseTime;

    QueryStats();
    void reset();
    string toString() const;
};

// Domain Statistics (per domain)
struct DomainStat {
    string domain;
    int queryCount;
    double avgResponseTime;
    time_t lastQueried;

    DomainStat();
    DomainStat(const string& dom);
    void update(double responseTime);
    bool operator<(const DomainStat& other) const {
        return domain < other.domain;
    }

    bool operator==(const DomainStat& other) const {
        return domain == other.domain;
    }
};

// Statistics Tracker
class StatsTracker {
private:
    QueryStats globalStats;
    HashTable<DomainStat>* domainStats;
    recursive_mutex statsMutex;

public:
    StatsTracker();
    ~StatsTracker();

    void recordQuery(const string& domain, bool success,
        bool fromCache, double responseTime);

    QueryStats getGlobalStats();
    DomainStat getDomainStats(const string& domain);
    vector<DomainStat> getTopDomains(int count = 10);

    void displayGlobalStats();
    void displayTopDomains(int count = 10);

    void reset();

    void saveToFile(const string& filename);
    void loadFromFile(const string& filename);
};

#endif // STATS_TRACKER_H