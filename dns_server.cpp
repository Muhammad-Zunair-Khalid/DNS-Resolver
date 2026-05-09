#include "dns_server.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;

// ==================== ServerConfig Implementation ====================

ServerConfig::ServerConfig() {
    serverIP = "0.0.0.0";
    serverPort = 5353;
    numWorkerThreads = 4;
    maxQueueSize = 100;
    cacheSize = 1000;
    dbPath = "./dns_db";
    externalDNS = "8.8.8.8";
}

// ==================== DNSServer Implementation ====================

DNSServer::DNSServer(const ServerConfig& cfg) : config(cfg) {
    database = new DatabaseEngine(config.dbPath);
    cache = new DNSCache(config.cacheSize);
    requestQueue = new RequestQueue(config.maxQueueSize);
    stats = new StatsTracker();
    isRunning = false;
    serverSocket = -1;
}

DNSServer::~DNSServer() {
    stop();

    delete database;
    delete cache;
    delete requestQueue;
    delete stats;
}

bool DNSServer::initialize() {
    cout << "\n========== INITIALIZING DNS SERVER ==========" << endl;
    cout << "Database Path: " << config.dbPath << endl;
    cout << "Cache Size: " << config.cacheSize << endl;
    cout << "External DNS: " << config.externalDNS << endl;

    if (!database->initialize()) {
        cerr << "ERROR: Failed to initialize database" << endl;
        return false;
    }

    cout << "Database initialized successfully" << endl;
    cout << "============================================\n" << endl;
    return true;
}

bool DNSServer::start() {
    if (isRunning) {
        cout << "Server is already running" << endl;
        return false;
    }

    cout << "\n========== STARTING DNS SERVER ==========" << endl;
    cout << "Server IP: " << config.serverIP << endl;
    cout << "Server Port: " << config.serverPort << endl;
    cout << "Worker Threads: " << config.numWorkerThreads << endl;
    cout << "Max Queue Size: " << config.maxQueueSize << endl;

    isRunning = true;

    // Start worker threads
    for (int i = 0; i < config.numWorkerThreads; i++) {
        workerThreads.emplace_back(&DNSServer::workerThreadFunc, this);
        cout << "Started worker thread #" << (i + 1) << endl;
    }

    cout << "DNS Server started successfully!" << endl;
    cout << "=========================================\n" << endl;
    return true;
}

void DNSServer::stop() {
    if (!isRunning) {
        return;
    }

    cout << "\n========== STOPPING DNS SERVER ==========" << endl;

    isRunning = false;
    requestQueue->shutdown();

    // Wait for all worker threads to finish
    cout << "Waiting for worker threads to finish..." << endl;
    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    workerThreads.clear();

    // Save all data to disk
    cout << "Saving data to disk..." << endl;
    database->flush();
    cache->saveToFile("cache_metadata.dat");
    stats->saveToFile("stats.dat");

    cout << "DNS Server stopped successfully" << endl;
    cout << "=========================================\n" << endl;
}

bool DNSServer::isServerRunning() const {
    return isRunning;
}

void DNSServer::workerThreadFunc() {
    cout << "[WORKER] Thread started (ID: " << this_thread::get_id() << ")" << endl;

    while (isRunning) {
        DNSRequest request;

        // Blocking dequeue - waits for requests
        if (!requestQueue->dequeue(request)) {
            // Queue shutdown
            break;
        }

        // Process the request
        auto startTime = chrono::high_resolution_clock::now();
        DNSResponse response = processQuery(request);
        auto endTime = chrono::high_resolution_clock::now();

        response.responseTime = chrono::duration<double, milli>(endTime - startTime).count();

        cout << "[WORKER] Processed: " << request.domain << " -> "
            << (response.success ? "SUCCESS" : "FAILED")
            << " (" << response.responseTime << " ms)" << endl;
    }

    cout << "[WORKER] Thread stopped (ID: " << this_thread::get_id() << ")" << endl;
}

DNSResponse DNSServer::processQuery(const DNSRequest& request) {
    DNSResponse response;
    response.requestID = request.requestID;

    string domain = Utils::normalizeDomain(request.domain);

    cout << "\n[QUERY] Processing: " << domain << endl;

    // ========== STEP 1: CHECK CACHE (Hash Table - O(1)) ==========
    cout << "[STEP 1] Checking cache..." << endl;
    DNSRecord cachedRecord;
    if (cache->get(domain, cachedRecord)) {
        cout << "[CACHE HIT] Found in cache!" << endl;

        response.success = true;
        response.record = cachedRecord;
        response.fromCache = true;

        stats->recordQuery(domain, true, true, 2.0);
        return response;
    }
    cout << "[CACHE MISS] Not in cache" << endl;

    // ========== STEP 2: CHECK DATABASE (B-Tree - O(log n)) ==========
    cout << "[STEP 2] Checking database..." << endl;
    DNSRecord dbRecord;
    if (database->getRecord(domain, dbRecord)) {
        cout << "[DATABASE HIT] Found in B-Tree!" << endl;

        // Cache it for future queries
        cache->put(domain, dbRecord);
        cout << "[CACHE] Stored in cache for future queries" << endl;

        response.success = true;
        response.record = dbRecord;
        response.fromCache = false;

        stats->recordQuery(domain, true, false, 10.0);
        return response;
    }
    cout << "[DATABASE MISS] Not in database" << endl;

    // ========== STEP 3: QUERY EXTERNAL DNS (8.8.8.8) ==========
    cout << "[STEP 3] Querying external DNS server..." << endl;
    DNSRecord resolvedRecord = resolveAndStore(domain);

    if (!resolvedRecord.values.empty()) {
        cout << "[EXTERNAL DNS] Successfully resolved!" << endl;

        response.success = true;
        response.record = resolvedRecord;
        response.fromCache = false;

        stats->recordQuery(domain, true, false, 50.0);
    }
    else {
        cout << "[EXTERNAL DNS] Failed to resolve domain" << endl;

        response.success = false;
        response.errorMessage = "Domain not found";

        stats->recordQuery(domain, false, false, 50.0);
    }

    return response;
}

DNSRecord DNSServer::resolveAndStore(const string& domain) {
    cout << "[EXTERNAL DNS] Sending UDP query to " << config.externalDNS << endl;

    // Query external DNS using real UDP packets
    string ip = DNSProtocol::queryExternalDNS(domain, config.externalDNS);

    DNSRecord record;

    if (!ip.empty() && Utils::isValidIPv4(ip)) {
        cout << "[EXTERNAL DNS] Resolved: " << domain << " -> " << ip << endl;

        // Create DNS record
        record = DNSRecord(domain, A_RECORD, ip, 300);

        // Store in database (B-Tree) - PERMANENT STORAGE
        cout << "[DATABASE] Storing in B-Tree (permanent)..." << endl;
        database->addRecord(record);

        // Store in cache (Hash Table) - FAST ACCESS
        cout << "[CACHE] Storing in Hash Table (cache)..." << endl;
        cache->put(domain, record);

        cout << "[SUCCESS] Domain stored in both database and cache" << endl;
    }
    else {
        cout << "[FAILURE] Could not resolve domain: " << domain << endl;
    }

    return record;
}

bool DNSServer::addDNSRecord(const DNSRecord& record) {
    cout << "[ADD RECORD] Adding: " << record.domain << endl;

    bool success = database->addRecord(record);

    if (success) {
        cache->put(record.domain, record);
        cout << "[ADD RECORD] Successfully added and cached" << endl;
    }
    else {
        cout << "[ADD RECORD] Failed to add record" << endl;
    }

    return success;
}

bool DNSServer::addDNSRecord(const string& domain, const string& ip) {
    DNSRecord record(domain, A_RECORD, ip);
    return addDNSRecord(record);
}

bool DNSServer::addDNSRecord(const string& domain, const set<string>& ips) {
    DNSRecord record(domain, A_RECORD, ips);
    return addDNSRecord(record);
}

bool DNSServer::updateDNSRecord(const DNSRecord& record) {
    cout << "[UPDATE RECORD] Updating: " << record.domain << endl;

    bool success = database->updateRecord(record);

    if (success) {
        cache->put(record.domain, record);
        cout << "[UPDATE RECORD] Successfully updated" << endl;
    }

    return success;
}

bool DNSServer::deleteDNSRecord(const string& domain) {
    cout << "[DELETE RECORD] Deleting: " << domain << endl;

    cache->remove(domain);
    bool success = database->deleteRecord(domain);

    if (success) {
        cout << "[DELETE RECORD] Successfully deleted" << endl;
    }

    return success;
}

void DNSServer::displayStatistics() {
    cout << "\n========== DNS SERVER STATISTICS ==========" << endl;

    // Global query statistics
    stats->displayGlobalStats();

    // Database statistics
    database->displayStatistics();

    // Cache statistics
    cache->displayStats();

    // Top domains
    stats->displayTopDomains(10);

    cout << "==========================================\n" << endl;
}

QueryStats DNSServer::getStats() {
    return stats->getGlobalStats();
}

void DNSServer::clearCache() {
    cout << "[CACHE] Clearing cache..." << endl;
    cache->clear();
    cout << "[CACHE] Cache cleared successfully" << endl;
}

void DNSServer::displayCache() {
    cout << "\n========== CACHE CONTENTS ==========" << endl;
    cache->display();
    cout << "====================================\n" << endl;
}

bool DNSServer::testExternalDNS(const string& domain) {
    cout << "\n========== TESTING EXTERNAL DNS ==========" << endl;
    cout << "Domain: " << domain << endl;
    cout << "DNS Server: " << config.externalDNS << endl;

    string ip = DNSProtocol::queryExternalDNS(domain, config.externalDNS);

    if (!ip.empty()) {
        cout << "+ SUCCESS: " << domain << " -> " << ip << endl;
        cout << "==========================================\n" << endl;
        return true;
    }
    else {
        cout << "- FAILED: Could not resolve " << domain << endl;
        cout << "==========================================\n" << endl;
        return false;
    }
}

bool DNSServer::loadRecordsFromFile(const string& filename) {
    cout << "\n========== LOADING DNS RECORDS ==========" << endl;
    cout << "File: " << filename << endl;

    vector<string> lines = Utils::readLines(filename);

    if (lines.empty()) {
        cout << "File not found or empty" << endl;
        return false;
    }

    int loaded = 0;
    int skipped = 0;

    for (const string& line : lines) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Format: domain,ip
        vector<string> parts = Utils::split(line, ',');

        if (parts.size() >= 2) {
            string domain = Utils::trim(parts[0]);
            string ip = Utils::trim(parts[1]);

            if (Utils::isValidDomain(domain) && Utils::isValidIPv4(ip)) {
                if (addDNSRecord(domain, ip)) {
                    loaded++;
                }
                else {
                    skipped++;
                }
            }
            else {
                cout << "Skipping invalid entry: " << line << endl;
                skipped++;
            }
        }
    }

    cout << "Loaded: " << loaded << " records" << endl;
    cout << "Skipped: " << skipped << " records" << endl;
    cout << "=========================================\n" << endl;

    return loaded > 0;
}

void DNSServer::listenForRequests() {
    // This would implement actual network socket listening
    // For now, it's a placeholder since we're using the queue directly
    cout << "[NETWORK] Server listening on " << config.serverIP << ":" << config.serverPort << endl;
}

void DNSServer::handleClientRequest(const string& clientIP, int clientPort, const string& query) {
    // Create DNS request
    DNSRequest request(query, clientIP, clientPort);

    // Enqueue request
    if (requestQueue->enqueue(request)) {
        cout << "[NETWORK] Request enqueued: " << query << " from " << clientIP << endl;
    }
    else {
        cout << "[NETWORK] Queue full! Request rejected" << endl;
    }
}

void DNSServer::sendResponse(const string& clientIP, int clientPort, const DNSResponse& response) {
    // This would send actual network response
    cout << "[NETWORK] Sending response to " << clientIP << ":" << clientPort << endl;
    cout << "[NETWORK] " << response.toString() << endl;
}