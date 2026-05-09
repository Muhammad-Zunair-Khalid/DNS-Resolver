#ifndef REQUEST_QUEUE_H
#define REQUEST_QUEUE_H

#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include <set>
#include <sstream>
#include <ctime>
#include "dns_record.h"

using namespace std;

// DNS Query Request
struct DNSRequest {
    string domain;
    string clientIP;
    int clientPort;
    time_t timestamp;
    int requestID;

    DNSRequest();
    DNSRequest(const string& dom, const string& ip, int port);

    string toString() const;
};

// DNS Query Response
struct DNSResponse {
    int requestID;
    bool success;
    DNSRecord record;
    string errorMessage;
    bool fromCache;
    double responseTime;

    DNSResponse();
    DNSResponse(int reqID, const DNSRecord& rec, bool cached);

    string toString() const;
};

// Thread-safe Request Queue
class RequestQueue {
private:
    queue<DNSRequest> queue;
    mutex  mtx;
    condition_variable cv;
    int maxQueueSize;
    bool shutdownFlag;

public:
    RequestQueue(int maxSize = 100);

    bool enqueue(const DNSRequest& request);
    bool dequeue(DNSRequest& request);
    bool tryDequeue(DNSRequest& request);

    int size();
    bool isEmpty();
    bool isFull();

    void shutdown();
    bool isShutdown() const;

    void clear();
};

#endif // REQUEST_QUEUE_H