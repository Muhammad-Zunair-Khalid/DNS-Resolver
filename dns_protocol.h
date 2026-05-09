#pragma once
#ifndef DNS_PROTOCOL_H
#define DNS_PROTOCOL_H

#include <string>
#include <vector>
#include <cstdint>
using namespace std;
// DNS Header Structure (12 bytes)
struct DNSHeader {
    uint16_t id;           // Transaction ID
    uint16_t flags;        // Flags
    uint16_t qdCount;      // Number of questions
    uint16_t anCount;      // Number of answers
    uint16_t nsCount;      // Number of authority records
    uint16_t arCount;      // Number of additional records

    DNSHeader();
    std::vector<uint8_t> serialize() const;
};

// DNS Question Structure
struct DNSQuestion {
    std::string qname;     // Domain name
    uint16_t qtype;        // Query type (A = 1)
    uint16_t qclass;       // Query class (IN = 1)

    DNSQuestion(const std::string& domain);
    std::vector<uint8_t> serialize() const;
};

// DNS Answer Structure
struct DNSAnswer {
    std::string name;
    uint16_t type;
    uint16_t cls;
    uint32_t ttl;
    uint16_t dataLength;
    std::vector<uint8_t> rdata;  // IP address (4 bytes for A record)

    DNSAnswer();
    std::string getIPAddress() const;
};

// DNS Protocol Handler
class DNSProtocol {
public:
    // Main function: Query external DNS and return IP
    static std::string queryExternalDNS(const std::string& domain,
        const std::string& dnsServer = "8.8.8.8",
        int port = 53);

    // Build complete DNS query packet
    static std::vector<uint8_t> buildDNSQuery(const std::string& domain);

    // Parse DNS response packet and extract IP
    static std::string parseDNSResponse(const std::vector<uint8_t>& response);

    // Parse multiple answers (if multiple IPs)
    static std::vector<std::string> parseAllAnswers(const std::vector<uint8_t>& response);

    // Encode domain name to DNS wire format
    // Example: "google.com" → [6]google[3]com[0]
    static std::vector<uint8_t> encodeDomainName(const std::string& domain);

    // Decode domain name from DNS wire format
    static std::string decodeDomainName(const std::vector<uint8_t>& packet,
        size_t& offset);

    // Send UDP query to DNS server
    static std::vector<uint8_t> sendUDPQuery(const std::string& server,
        int port,
        const std::vector<uint8_t>& query);

    // Parse DNS header from response
    static DNSHeader parseHeader(const std::vector<uint8_t>& response);

    // Skip question section in response
    static void skipQuestion(const std::vector<uint8_t>& packet, size_t& offset);

    // Parse single answer record
    static DNSAnswer parseAnswer(const std::vector<uint8_t>& packet, size_t& offset);

    // Convert 4 bytes to IP string (e.g., [142,250,185,46] → "142.250.185.46")
    static std::string bytesToIP(const std::vector<uint8_t>& bytes);

    // Generate random transaction ID
    static uint16_t generateTransactionID();
};

#endif // DNS_PROTOCOL_H