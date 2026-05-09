#include "dns_record.h"
#include <iostream>
#include <sstream>
#include <algorithm>

using namespace std;

int DNSRecord::roundRobinIndex = 0;

// ========== DNSRecord Implementation ==========

DNSRecord::DNSRecord() {
    domain = "";
    type = A_RECORD;
    ttl = 300;
    priority = 0;
    createdAt = time(nullptr);
}

DNSRecord::DNSRecord(const string& domain, DNSRecordType type,
    const string& value, int ttl) {
    this->domain = domain;
    this->type = type;
    this->ttl = ttl;
    this->priority = 0;
    this->createdAt = time(nullptr);
    values.insert(value);
}

DNSRecord::DNSRecord(const string& domain, DNSRecordType type,
    const set<string>& values, int ttl) {
    this->domain = domain;
    this->type = type;
    this->values = values;
    this->ttl = ttl;
    this->priority = 0;
    this->createdAt = time(nullptr);
}

void DNSRecord::addValue(const string& value) {
    values.insert(value);
}

string DNSRecord::getValue() const {
    if (values.empty()) return "";
    return *values.begin();
}

set<string> DNSRecord::getAllValues() const {
    return values;
}

string DNSRecord::getValueRoundRobin() {
    if (values.empty()) return "";

    int size = values.size();
    roundRobinIndex = roundRobinIndex % size;

    auto it = values.begin();
    advance(it, roundRobinIndex);

    roundRobinIndex++;
    return *it;
}

bool DNSRecord::hasValue(const string& value) const {
    return values.find(value) != values.end();
}

int DNSRecord::getValueCount() const {
    return values.size();
}

string DNSRecord::serialize() const {
    stringstream ss;
    ss << domain << "|" << type << "|" << ttl << "|" << priority << "|" << createdAt << "|";

    // Serialize all IPs
    for (const auto& ip : values) {
        ss << ip << ",";
    }

    return ss.str();
}

DNSRecord DNSRecord::deserialize(const string& data) {
    DNSRecord record;
    stringstream ss(data);
    string token;

    // Parse domain
    getline(ss, record.domain, '|');

    // Parse type
    int typeInt;
    ss >> typeInt;
    ss.ignore();
    record.type = static_cast<DNSRecordType>(typeInt);

    // Parse ttl
    ss >> record.ttl;
    ss.ignore();

    // Parse priority
    ss >> record.priority;
    ss.ignore();

    // Parse createdAt
    long long timeValue;
    ss >> timeValue;
    ss.ignore();
    record.createdAt = static_cast<time_t>(timeValue);

    // Parse IPs
    string ipsStr;
    getline(ss, ipsStr, '|');
    stringstream ipStream(ipsStr);
    string ip;
    while (getline(ipStream, ip, ',')) {
        if (!ip.empty()) {
            record.values.insert(ip);
        }
    }

    return record;
}

string DNSRecord::toString() const {
    stringstream ss;
    ss << "Domain: " << domain << "\n";
    ss << "Type: " << typeToString(type) << "\n";
    ss << "IPs: ";
    for (const auto& ip : values) {
        ss << ip << " ";
    }
    ss << "\nTTL: " << ttl << "s\n";
    ss << "Created: " << createdAt;
    return ss.str();
}

string DNSRecord::typeToString(DNSRecordType type) {
    switch (type) {
    case A_RECORD: return "A";
    case AAAA_RECORD: return "AAAA";
    case CNAME_RECORD: return "CNAME";
    case MX_RECORD: return "MX";
    case TXT_RECORD: return "TXT";
    default: return "UNKNOWN";
    }
}

DNSRecordType DNSRecord::stringToType(const string& typeStr) {
    if (typeStr == "A") return A_RECORD;
    if (typeStr == "AAAA") return AAAA_RECORD;
    if (typeStr == "CNAME") return CNAME_RECORD;
    if (typeStr == "MX") return MX_RECORD;
    if (typeStr == "TXT") return TXT_RECORD;
    return A_RECORD; // Default
}

// ========== CacheEntry Implementation ==========

CacheEntry::CacheEntry() {
    cachedAt = time(nullptr);
    expiresAt = cachedAt + 300; // Default 5 min TTL
    hitCount = 0;
}

CacheEntry::CacheEntry(const DNSRecord& rec) {
    record = rec;
    cachedAt = time(nullptr);
    expiresAt = cachedAt + rec.ttl;
    hitCount = 0;
}

bool CacheEntry::isExpired() const {
    return time(nullptr) > expiresAt;
}

void CacheEntry::refresh() {
    cachedAt = time(nullptr);
    expiresAt = cachedAt + record.ttl;
}

string CacheEntry::serialize() const {
    stringstream ss;
    ss << record.serialize() << "||";
    ss << cachedAt << "|" << expiresAt << "|" << hitCount;
    return ss.str();
}

CacheEntry CacheEntry::deserialize(const string& data) {
    CacheEntry entry;

    size_t separator = data.find("||");
    if (separator == string::npos) return entry;

    // Parse DNSRecord part
    string recordStr = data.substr(0, separator);
    entry.record = DNSRecord::deserialize(recordStr);

    // Parse cache metadata
    string metaStr = data.substr(separator + 2);
    stringstream ss(metaStr);

    long long timeValue;
    ss >> timeValue;
    ss.ignore();
    entry.cachedAt = static_cast<time_t>(timeValue);

    ss >> timeValue;
    ss.ignore();
    entry.expiresAt = static_cast<time_t>(timeValue);

    ss >> entry.hitCount;

    return entry;
}