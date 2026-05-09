#include "request_queue.h"
#include <iostream>

using namespace std;

// ==================== DNSRequest Implementation ====================

DNSRequest::DNSRequest() {
    domain = "";
    clientIP = "";
    clientPort = 0;
    timestamp = time(nullptr);
    requestID = 0;
}

DNSRequest::DNSRequest(const string& dom, const string& ip, int port) {
    domain = dom;
    clientIP = ip;
    clientPort = port;
    timestamp = time(nullptr);
    requestID = rand() % 100000;
}

string DNSRequest::toString() const {
    stringstream ss;
    ss << "Request #" << requestID << " - Domain: " << domain
        << " from " << clientIP << ":" << clientPort;
    return ss.str();
}

// ==================== DNSResponse Implementation ====================

DNSResponse::DNSResponse() {
    requestID = 0;
    success = false;
    errorMessage = "";
    fromCache = false;
    responseTime = 0.0;
}

DNSResponse::DNSResponse(int reqID, const DNSRecord& rec, bool cached) {
    requestID = reqID;
    record = rec;
    success = true;
    fromCache = cached;
    errorMessage = "";
    responseTime = 0.0;
}

string DNSResponse::toString() const {
    stringstream ss;
    if (success) {
        cout << "Success - " << record.domain << " -> ";
        for (const auto& ip : record.values) {
            ss << ip << " ";
        }
        ss << (fromCache ? "(cached)" : "(fresh)");
    }
    else {
        ss << "Failed - " << errorMessage;
    }
    return ss.str();
}

// ==================== RequestQueue Implementation ====================

RequestQueue::RequestQueue(int maxSize) : mtx(), cv() {
    maxQueueSize = maxSize;
    shutdownFlag = false;
}

bool RequestQueue::enqueue(const DNSRequest& request) {
    unique_lock<mutex> lock(mtx);

    if (shutdownFlag) {
        return false;
    }

    if (queue.size() >= maxQueueSize) {
        cerr << "Queue full! Rejecting request." << endl;
        return false;
    }

    queue.push(request);
    cv.notify_one();
    return true;
}

bool RequestQueue::dequeue(DNSRequest& request) {
    unique_lock<mutex > lock(mtx);

    while (queue.empty() && !shutdownFlag) {
        cv.wait(lock);
    }

    if (shutdownFlag && queue.empty()) {
        return false;
    }

    request = queue.front();
    queue.pop();
    return true;
}

bool RequestQueue::tryDequeue(DNSRequest& request) {
    unique_lock<mutex > lock(mtx);

    if (queue.empty()) {
        return false;
    }

    request = queue.front();
    queue.pop();
    return true;
}

int RequestQueue::size() {
    lock_guard<mutex > lock(mtx);
    return queue.size();
}

bool RequestQueue::isEmpty() {
    lock_guard<mutex > lock(mtx);
    return queue.empty();
}

bool RequestQueue::isFull() {
    lock_guard<mutex > lock(mtx);
    return queue.size() >= maxQueueSize;
}

void RequestQueue::shutdown() {
    lock_guard<mutex > lock(mtx);
    shutdownFlag = true;
    cv.notify_all();
}

bool RequestQueue::isShutdown() const {
    return shutdownFlag;
}

void RequestQueue::clear() {
    lock_guard<mutex > lock(mtx);
    while (!queue.empty()) {
        queue.pop();
    }
}