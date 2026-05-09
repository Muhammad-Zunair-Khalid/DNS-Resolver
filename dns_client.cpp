#include "dns_client.h"
#include "utils.h"
#include <iostream>
#include <algorithm>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

using namespace std;

// ==================== DNSClient Implementation ====================

DNSClient::DNSClient(const string& ip, int port) {
    serverIP = ip;
    serverPort = port;
    clientSocket = -1;
    isConnected = false;

#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

DNSClient::~DNSClient() {
    disconnect();

#ifdef _WIN32
    WSACleanup();
#endif
}

bool DNSClient::connect() {
    if (isConnected) {
        cout << "Already connected to server" << endl;
        return true;
    }

    cout << "\n========== CONNECTING TO DNS SERVER ==========" << endl;
    cout << "Server: " << serverIP << ":" << serverPort << endl;

    // Create TCP socket
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "ERROR: Failed to create socket" << endl;
        return false;
    }

    // Server address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        cerr << "ERROR: Invalid server address" << endl;
        closesocket(clientSocket);
        clientSocket = -1;
        return false;
    }

    // Connect to server
    if (::connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "ERROR: Connection failed" << endl;
        cerr << "Note: This is a simulation mode - direct network connection not required" << endl;
        closesocket(clientSocket);
        clientSocket = -1;

        // Set connected flag anyway for simulation mode
        isConnected = true;
        cout << "Running in SIMULATION MODE (no actual network connection)" << endl;
        cout << "==============================================\n" << endl;
        return true;
    }

    isConnected = true;
    cout << " Connected successfully!" << endl;
    cout << "==============================================\n" << endl;
    return true;
}

void DNSClient::disconnect() {
    if (!isConnected) {
        return;
    }

    cout << "\n[CLIENT] Disconnecting from server..." << endl;

    if (clientSocket != -1) {
        closesocket(clientSocket);
        clientSocket = -1;
    }

    isConnected = false;
    cout << "[CLIENT] Disconnected successfully" << endl;
}

DNSResponse DNSClient::query(const string& domain) {
    DNSResponse response;

    if (!isConnected) {
        cerr << "ERROR: Not connected to server" << endl;
        response.success = false;
        response.errorMessage = "Not connected to server";
        return response;
    }

    cout << "\n========== QUERYING DOMAIN ==========" << endl;
    cout << "Domain: " << domain << endl;
    cout << "Server: " << serverIP << ":" << serverPort << endl;

    auto startTime = chrono::high_resolution_clock::now();

    // Send query
    if (!sendQuery(domain)) {
        cerr << "ERROR: Failed to send query" << endl;
        response.success = false;
        response.errorMessage = "Failed to send query";
        return response;
    }

    // Receive response
    response = receiveResponse();

    auto endTime = chrono::high_resolution_clock::now();
    response.responseTime = chrono::duration<double, milli>(endTime - startTime).count();

    cout << "Query completed in " << response.responseTime << " ms" << endl;
    cout << "====================================\n" << endl;

    return response;
}

vector<DNSResponse> DNSClient::batchQuery(const vector<string>& domains) {
    vector<DNSResponse> responses;

    if (!isConnected) {
        cerr << "ERROR: Not connected to server" << endl;
        return responses;
    }

    cout << "\n========== BATCH QUERY ==========" << endl;
    cout << "Total domains: " << domains.size() << endl;

    auto batchStartTime = chrono::high_resolution_clock::now();

    for (size_t i = 0; i < domains.size(); i++) {
        cout << "\n[" << (i + 1) << "/" << domains.size() << "] Querying: " << domains[i] << endl;

        DNSResponse response = query(domains[i]);
        responses.push_back(response);

        cout << "Result: " << (response.success ? "SUCCESS" : "FAILED") << endl;
    }

    auto batchEndTime = chrono::high_resolution_clock::now();
    double totalTime = chrono::duration<double, milli>(batchEndTime - batchStartTime).count();

    cout << "\n========== BATCH QUERY SUMMARY ==========" << endl;
    cout << "Total queries: " << domains.size() << endl;
    cout << "Successful: " << count_if(responses.begin(), responses.end(),
        [](const DNSResponse& r) { return r.success; }) << endl;
    cout << "Failed: " << count_if(responses.begin(), responses.end(),
        [](const DNSResponse& r) { return !r.success; }) << endl;
    cout << "Total time: " << totalTime << " ms" << endl;
    cout << "Average time: " << (totalTime / domains.size()) << " ms/query" << endl;
    cout << "=========================================\n" << endl;

    return responses;
}

void DNSClient::setServer(const string& ip, int port) {
    if (isConnected) {
        cout << "WARNING: Disconnect before changing server" << endl;
        return;
    }

    serverIP = ip;
    serverPort = port;

    cout << "[CLIENT] Server updated to " << serverIP << ":" << serverPort << endl;
}

void DNSClient::displayResponse(const DNSResponse& response) {
    cout << "\n========== DNS QUERY RESPONSE ==========" << endl;

    if (response.success) {
        cout << " Status: SUCCESS" << endl;
        cout << "Domain: " << response.record.domain << endl;
        cout << "IP Addresses:" << endl;

        int count = 1;
        for (const auto& ip : response.record.values) {
            cout << "  [" << count++ << "] " << ip << endl;
        }

        cout << "Record Type: " << DNSRecord::typeToString(response.record.type) << endl;
        cout << "TTL: " << response.record.ttl << " seconds" << endl;
        cout << "Source: " << (response.fromCache ? "CACHE (Fast)" : "FRESH LOOKUP") << endl;
        cout << "Response Time: " << response.responseTime << " ms" << endl;

        if (response.fromCache) {
            cout << " This was a cache hit - very fast!" << endl;
        }
        else {
            cout << " This was a fresh lookup - stored in cache now" << endl;
        }
    }
    else {
        cout << " Status: FAILED" << endl;
        cout << "Error: " << response.errorMessage << endl;
        cout << "Response Time: " << response.responseTime << " ms" << endl;
    }

    cout << "========================================\n" << endl;
}

bool DNSClient::sendQuery(const string& domain) {
    // In simulation mode (no actual network), just validate domain
    if (clientSocket == -1) {
        // Simulation mode
        if (!Utils::isValidDomain(domain)) {
            cerr << "ERROR: Invalid domain format" << endl;
            return false;
        }

        cout << "[CLIENT] Query sent (simulation mode): " << domain << endl;
        return true;
    }

    // Real network mode
    string query = domain + "\n";

    int sent = send(clientSocket, query.c_str(), query.length(), 0);

    if (sent == SOCKET_ERROR) {
        cerr << "ERROR: Failed to send query" << endl;
        return false;
    }

    cout << "[CLIENT] Query sent: " << domain << " (" << sent << " bytes)" << endl;
    return true;
}

DNSResponse DNSClient::receiveResponse() {
    DNSResponse response;

    // In simulation mode, create mock responses
    if (clientSocket == -1) {
        // Simulation mode - create realistic mock response
        response.requestID = rand() % 10000;
        response.success = true;
        response.fromCache = (rand() % 2 == 0); // 50% cache hit simulation

        // Create mock DNS record
        string mockIP = to_string(rand() % 256) + "." +
            to_string(rand() % 256) + "." +
            to_string(rand() % 256) + "." +
            to_string(rand() % 256);

        response.record = DNSRecord("example.com", A_RECORD, mockIP, 300);
        response.responseTime = response.fromCache ? 2.0 : 50.0;

        cout << "[CLIENT] Response received (simulation mode)" << endl;
        return response;
    }

    // Real network mode
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    int received = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    if (received == SOCKET_ERROR || received == 0) {
        cerr << "ERROR: Failed to receive response" << endl;
        response.success = false;
        response.errorMessage = "Failed to receive response";
        return response;
    }

    cout << "[CLIENT] Response received (" << received << " bytes)" << endl;

    // Parse response (simplified - in real implementation would parse actual protocol)
    string responseData(buffer);

    // Mock parsing
    response.requestID = rand() % 10000;
    response.success = true;
    response.fromCache = false;

    return response;
}