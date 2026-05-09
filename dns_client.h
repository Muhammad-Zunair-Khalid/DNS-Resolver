#pragma once
#ifndef DNS_CLIENT_H
#define DNS_CLIENT_H

#include "dns_record.h"
#include "request_queue.h"
#include <string>
using namespace std;
// DNS Client for testing
class DNSClient {
private:
    std::string serverIP;
    int serverPort;
    int clientSocket;
    bool isConnected;

public:
    DNSClient(const std::string& ip = "127.0.0.1", int port = 53);
    ~DNSClient();

    // Connection
    bool connect();
    void disconnect();

    // Query
    DNSResponse query(const std::string& domain);

    // Batch queries (for testing multiple clients)
    std::vector<DNSResponse> batchQuery(const std::vector<std::string>& domains);

    // Configuration
    void setServer(const std::string& ip, int port);

    // Display response
    void displayResponse(const DNSResponse& response);

private:
    // Send query to server
    bool sendQuery(const std::string& domain);

    // Receive response from server
    DNSResponse receiveResponse();
};

#endif // DNS_CLIENT_H