#include "dns_protocol.h"
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>

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

// ==================== DNSHeader Implementation ====================

DNSHeader::DNSHeader() {
    id = 0;
    flags = 0x0100;  // Standard query, recursion desired
    qdCount = 1;     // 1 question
    anCount = 0;
    nsCount = 0;
    arCount = 0;
}

vector<uint8_t> DNSHeader::serialize() const {
    vector<uint8_t> data(12);

    // Transaction ID (2 bytes)
    data[0] = (id >> 8) & 0xFF;
    data[1] = id & 0xFF;

    // Flags (2 bytes)
    data[2] = (flags >> 8) & 0xFF;
    data[3] = flags & 0xFF;

    // Question count (2 bytes)
    data[4] = (qdCount >> 8) & 0xFF;
    data[5] = qdCount & 0xFF;

    // Answer count (2 bytes)
    data[6] = (anCount >> 8) & 0xFF;
    data[7] = anCount & 0xFF;

    // Authority count (2 bytes)
    data[8] = (nsCount >> 8) & 0xFF;
    data[9] = nsCount & 0xFF;

    // Additional count (2 bytes)
    data[10] = (arCount >> 8) & 0xFF;
    data[11] = arCount & 0xFF;

    return data;
}
string DNSProtocol::bytesToIP(const vector<uint8_t>& bytes) {
    if (bytes.size() < 4) {
        return "";
    }

    stringstream ss;
    ss << static_cast<int>(bytes[0]) << "."
        << static_cast<int>(bytes[1]) << "."
        << static_cast<int>(bytes[2]) << "."
        << static_cast<int>(bytes[3]);

    return ss.str();
}









// ==================== DNSQuestion Implementation ====================

DNSQuestion::DNSQuestion(const string& domain) {
    qname = domain;
    qtype = 1;   // A record
    qclass = 1;  // IN (Internet)
}

vector<uint8_t> DNSQuestion::serialize() const {
    vector<uint8_t> data;

    // Encode domain name
    vector<uint8_t> encoded = DNSProtocol::encodeDomainName(qname);
    data.insert(data.end(), encoded.begin(), encoded.end());

    // Query type (2 bytes)
    data.push_back((qtype >> 8) & 0xFF);
    data.push_back(qtype & 0xFF);

    // Query class (2 bytes)
    data.push_back((qclass >> 8) & 0xFF);
    data.push_back(qclass & 0xFF);

    return data;
}

// ==================== DNSAnswer Implementation ====================

DNSAnswer::DNSAnswer() {
    name = "";
    type = 0;
    cls = 0;
    ttl = 0;
    dataLength = 0;
}

string DNSAnswer::getIPAddress() const {
    if (type == 1 && dataLength == 4 && rdata.size() >= 4) {
        return DNSProtocol::bytesToIP(rdata);
    }
    return "";
}

// ==================== DNSProtocol Implementation ====================

uint16_t DNSProtocol::generateTransactionID() {
    return static_cast<uint16_t>(rand() % 65536);
}

vector<uint8_t> DNSProtocol::encodeDomainName(const string& domain) {
    vector<uint8_t> encoded;
    stringstream ss(domain);
    string label;

    while (getline(ss, label, '.')) {
        if (label.length() > 63) {
            cerr << "Label too long: " << label << endl;
            continue;
        }

        // Length byte
        encoded.push_back(static_cast<uint8_t>(label.length()));

        // Label bytes
        for (char c : label) {
            encoded.push_back(static_cast<uint8_t>(c));
        }
    }

    // Null terminator
    encoded.push_back(0);

    return encoded;
}

string DNSProtocol::decodeDomainName(const vector<uint8_t>& packet, size_t& offset) {
    string domain;
    bool jumped = false;
    size_t originalOffset = offset;
    int jumps = 0;
    const int MAX_JUMPS = 5;

    while (offset < packet.size() && jumps < MAX_JUMPS) {
        uint8_t length = packet[offset];

        // End of name
        if (length == 0) {
            offset++;
            break;
        }

        // Compression pointer (starts with 11 in binary)
        if ((length & 0xC0) == 0xC0) {
            if (offset + 1 >= packet.size()) break;

            if (!jumped) {
                originalOffset = offset + 2;
            }

            // Get pointer offset
            uint16_t pointer = ((length & 0x3F) << 8) | packet[offset + 1];
            offset = pointer;
            jumped = true;
            jumps++;
            continue;
        }

        // Normal label
        offset++;

        if (!domain.empty()) {
            domain += ".";
        }

        for (int i = 0; i < length && offset < packet.size(); i++) {
            domain += static_cast<char>(packet[offset++]);
        }
    }

    // Restore offset if we jumped
    if (jumped) {
        offset = originalOffset;
    }

    return domain;
}



vector<uint8_t> DNSProtocol::buildDNSQuery(const string& domain) {
    vector<uint8_t> query;

    // Build header
    DNSHeader header;
    header.id = generateTransactionID();
    vector<uint8_t> headerData = header.serialize();
    query.insert(query.end(), headerData.begin(), headerData.end());

    // Build question
    DNSQuestion question(domain);
    vector<uint8_t> questionData = question.serialize();
    query.insert(query.end(), questionData.begin(), questionData.end());

    return query;
}

DNSHeader DNSProtocol::parseHeader(const vector<uint8_t>& response) {
    DNSHeader header;

    if (response.size() < 12) {
        return header;
    }

    header.id = (response[0] << 8) | response[1];
    header.flags = (response[2] << 8) | response[3];
    header.qdCount = (response[4] << 8) | response[5];
    header.anCount = (response[6] << 8) | response[7];
    header.nsCount = (response[8] << 8) | response[9];
    header.arCount = (response[10] << 8) | response[11];

    return header;
}

void DNSProtocol::skipQuestion(const vector<uint8_t>& packet, size_t& offset) {
    // Skip domain name
    decodeDomainName(packet, offset);

    // Skip QTYPE and QCLASS (4 bytes)
    offset += 4;
}

DNSAnswer DNSProtocol::parseAnswer(const vector<uint8_t>& packet, size_t& offset) {
    DNSAnswer answer;

    if (offset >= packet.size()) {
        return answer;
    }

    // Parse name (might be compressed)
    answer.name = decodeDomainName(packet, offset);

    if (offset + 10 > packet.size()) {
        return answer;
    }

    // Parse type (2 bytes)
    answer.type = (packet[offset] << 8) | packet[offset + 1];
    offset += 2;

    // Parse class (2 bytes)
    answer.cls = (packet[offset] << 8) | packet[offset + 1];
    offset += 2;

    // Parse TTL (4 bytes)
    answer.ttl = (packet[offset] << 24) | (packet[offset + 1] << 16) |
        (packet[offset + 2] << 8) | packet[offset + 3];
    offset += 4;

    // Parse data length (2 bytes)
    answer.dataLength = (packet[offset] << 8) | packet[offset + 1];
    offset += 2;

    // Parse rdata
    if (offset + answer.dataLength <= packet.size()) {
        answer.rdata.assign(packet.begin() + offset,
            packet.begin() + offset + answer.dataLength);
        offset += answer.dataLength;
    }

    return answer;
}

string DNSProtocol::parseDNSResponse(const vector<uint8_t>& response) {
    if (response.size() < 12) {
        return "";
    }

    DNSHeader header = parseHeader(response);

    if (header.anCount == 0) {
        return "";
    }

    // Skip header (12 bytes)
    size_t offset = 12;

    // Skip question section
    for (int i = 0; i < header.qdCount && offset < response.size(); i++) {
        skipQuestion(response, offset);
    }

    // Parse answers
    for (int i = 0; i < header.anCount && offset < response.size(); i++) {
        DNSAnswer answer = parseAnswer(response, offset);

        // If A record (type 1), return IP
        if (answer.type == 1) {
            string ip = answer.getIPAddress();
            if (!ip.empty()) {
                return ip;
            }
        }
    }

    return "";
}

vector<string> DNSProtocol::parseAllAnswers(const vector<uint8_t>& response) {
    vector<string> ips;

    if (response.size() < 12) {
        return ips;
    }

    DNSHeader header = parseHeader(response);

    if (header.anCount == 0) {
        return ips;
    }

    size_t offset = 12;

    // Skip questions
    for (int i = 0; i < header.qdCount && offset < response.size(); i++) {
        skipQuestion(response, offset);
    }

    // Parse all answers
    for (int i = 0; i < header.anCount && offset < response.size(); i++) {
        DNSAnswer answer = parseAnswer(response, offset);

        if (answer.type == 1) {
            string ip = answer.getIPAddress();
            if (!ip.empty()) {
                ips.push_back(ip);
            }
        }
    }

    return ips;
}

vector<uint8_t> DNSProtocol::sendUDPQuery(const string& server, int port,
    const vector<uint8_t>& query) {
    vector<uint8_t> response;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return response;
    }
#endif

    // Create UDP socket
    SOCKET sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == INVALID_SOCKET) {
        cerr << "Socket creation failed" << endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return response;
    }

    // Set timeout (5 seconds)
#ifdef _WIN32
    DWORD timeout = 5000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif

    // Setup server address
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, server.c_str(), &serverAddr.sin_addr) <= 0) {
        cerr << "Invalid DNS server address" << endl;
        closesocket(sockfd);
#ifdef _WIN32
        WSACleanup();
#endif
        return response;
    }

    // Send query
    int sent = sendto(sockfd, (const char*)query.data(), query.size(), 0,
        (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    if (sent == SOCKET_ERROR) {
        cerr << "Send failed" << endl;
        closesocket(sockfd);
#ifdef _WIN32
        WSACleanup();
#endif
        return response;
    }

    // Receive response
    uint8_t buffer[512];
    struct sockaddr_in fromAddr;
    socklen_t fromLen = sizeof(fromAddr);

    int received = recvfrom(sockfd, (char*)buffer, sizeof(buffer), 0,
        (struct sockaddr*)&fromAddr, &fromLen);

    closesocket(sockfd);
#ifdef _WIN32
    WSACleanup();
#endif

    if (received == SOCKET_ERROR || received < 12) {
        cerr << "Receive failed or incomplete response" << endl;
        return response;
    }

    // Copy to vector
    response.assign(buffer, buffer + received);

    return response;
}

string DNSProtocol::queryExternalDNS(const string& domain, const string& dnsServer, int port) {
    cout << "[DNS] Querying " << dnsServer << " for: " << domain << endl;

    // Build query packet
    vector<uint8_t> query = buildDNSQuery(domain);

    // Send UDP query
    vector<uint8_t> response = sendUDPQuery(dnsServer, port, query);

    if (response.empty()) {
        cerr << "[DNS] No response received" << endl;
        return "";
    }

    // Parse response
    string ip = parseDNSResponse(response);

    if (!ip.empty()) {
        cout << "[DNS] Resolved: " << domain << " -> " << ip << endl;
    }
    else {
        cerr << "[DNS] Failed to resolve: " << domain << endl;
    }

    return ip;
}