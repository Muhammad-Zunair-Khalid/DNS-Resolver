#ifndef DNS_SERVER_H
#define DNS_SERVER_H

#include "database_engine.h"
#include "dns_cache.h"
#include "request_queue.h"
#include "stats_tracker.h"
#include "dns_protocol.h"
#include <string>
#include <thread>
#include <vector>
#include <set>
using namespace std;
// Server Configuration
struct ServerConfig {
    std::string serverIP;
    int serverPort;
    int numWorkerThreads;
    int maxQueueSize;
    int cacheSize;
    std::string dbPath;
    std::string externalDNS;

    ServerConfig();
};

// DNS Server
class DNSServer {
private:
    ServerConfig config;
    DatabaseEngine* database;
    DNSCache* cache;
    RequestQueue* requestQueue;
    StatsTracker* stats;

    // Worker threads
    std::vector<std::thread> workerThreads;
    bool isRunning;

    // Socket
    int serverSocket;

public:
    DNSServer(const ServerConfig& cfg);
    ~DNSServer();

    // Server lifecycle
    bool start();
    void stop();
    bool isServerRunning() const;

    // Initialize
    bool initialize();

    // Load initial DNS records
    bool loadRecordsFromFile(const std::string& filename);

    //  MODIFIED: Query processing with multi-IP support
    // Step 1: Check cache (Hash Table)
    // Step 2: Check database (B-Tree)
    // Step 3: Query external DNS (8.8.8.8)
    DNSResponse processQuery(const DNSRequest& request);

    //  MODIFIED: Add DNS record (supports multiple IPs)
    bool addDNSRecord(const DNSRecord& record);
    bool addDNSRecord(const std::string& domain, const std::string& ip);  // Single IP
    bool addDNSRecord(const std::string& domain, const std::set<std::string>& ips);  // Multiple IPs

    bool updateDNSRecord(const DNSRecord& record);
    bool deleteDNSRecord(const std::string& domain);

    // Statistics
    void displayStatistics();
    QueryStats getStats();

    // Cache management
    void clearCache();
    void displayCache();

    // Test external DNS resolution
    bool testExternalDNS(const std::string& domain);

private:
    // Worker thread function
    void workerThreadFunc();

    // Network functions
    void listenForRequests();
    void handleClientRequest(const std::string& clientIP, int clientPort,
        const std::string& query);

    // Response helpers
    void sendResponse(const std::string& clientIP, int clientPort,
        const DNSResponse& response);

    // Resolve using external DNS and store in database
    DNSRecord resolveAndStore(const std::string& domain);
};

#endif // DNS_SERVER_H