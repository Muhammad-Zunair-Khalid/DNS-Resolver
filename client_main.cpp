#include "dns_client.h"
#include "dns_server.h"
#include "utils.h"
#include <iostream>
#include <algorithm>  
#include <thread>
#include <chrono>

using namespace std;

void printClientMenu() {
    cout << "\n========== DNS CLIENT MENU ==========" << endl;
    cout << "1. Query Single Domain" << endl;
    cout << "2. Batch Query (Multiple Domains)" << endl;
    cout << "3. Test Performance" << endl;
    cout << "4. Change Server" << endl;
    cout << "5. Reconnect to Server" << endl;
    cout << "0. Exit" << endl;
    cout << "====================================" << endl;
    cout << "Choice: ";
}

int main() {
    cout << "========== DNS RESOLVER CLIENT ==========" << endl;
    cout << "Student: Zunair Khalid (BSCS24097)" << endl;
    cout << "========================================\n" << endl;

    // Create client
    DNSClient client("127.0.0.1", 5353);

    // Connect to server
    if (!client.connect()) {
        cout << "Running in simulation mode (server not required)" << endl;
    }

    bool running = true;

    while (running) {
        printClientMenu();

        int choice;
        cin >> choice;
        cin.ignore();

        switch (choice) {
        case 1: {
            // Single query
            string domain;
            cout << "\nEnter domain name: ";
            getline(cin, domain);

            DNSResponse response = client.query(domain);
            client.displayResponse(response);
            break;
        }

        case 2: {
            // Batch query
            int count;
            cout << "\nHow many domains? ";
            cin >> count;
            cin.ignore();

            vector<string> domains;
            for (int i = 0; i < count; i++) {
                string domain;
                cout << "Domain " << (i + 1) << ": ";
                getline(cin, domain);
                domains.push_back(domain);
            }

            vector<DNSResponse> responses = client.batchQuery(domains);

            cout << "\nDisplaying detailed results:" << endl;
            for (size_t i = 0; i < responses.size(); i++) {
                cout << "\n--- Result " << (i + 1) << " ---" << endl;
                client.displayResponse(responses[i]);
            }
            break;
        }

        case 3: {
            // Performance test
            cout << "\n========== PERFORMANCE TEST ==========" << endl;

            vector<string> testDomains = {
                "google.com",
                "facebook.com",
                "youtube.com",
                "amazon.com",
                "wikipedia.org",
                "twitter.com",
                "instagram.com",
                "linkedin.com",
                "reddit.com",
                "github.com"
            };

            cout << "Testing with " << testDomains.size() << " popular domains..." << endl;

            vector<DNSResponse> responses = client.batchQuery(testDomains);

            cout << "\n========== PERFORMANCE RESULTS ==========" << endl;

            double totalTime = 0;
            int cacheHits = 0;

            for (const auto& resp : responses) {
                totalTime += resp.responseTime;
                if (resp.fromCache) cacheHits++;
            }

            cout << "Total queries: " << responses.size() << endl;
            cout << "Cache hits: " << cacheHits << endl;
            cout << "Cache miss: " << (responses.size() - cacheHits) << endl;
            cout << "Cache hit rate: " << (cacheHits * 100.0 / responses.size()) << "%" << endl;
            cout << "Average time: " << (totalTime / responses.size()) << " ms" << endl;
            cout << "=========================================\n" << endl;
            break;
        }

        case 4: {
            // Change server
            string ip;
            int port;

            cout << "\nEnter server IP: ";
            getline(cin, ip);
            cout << "Enter server port: ";
            cin >> port;
            cin.ignore();

            client.disconnect();
            client.setServer(ip, port);

            cout << "Server changed. Use option 5 to reconnect." << endl;
            break;
        }

        case 5: {
            // Reconnect
            client.disconnect();
            if (client.connect()) {
                cout << "Reconnected successfully!" << endl;
            }
            else {
                cout << "Reconnection failed (running in simulation mode)" << endl;
            }
            break;
        }

        case 0: {
            running = false;
            break;
        }

        default: {
            cout << "Invalid choice! Please try again." << endl;
            break;
        }
        }

        this_thread::sleep_for(chrono::milliseconds(500));
    }

    cout << "\nDisconnecting..." << endl;
    client.disconnect();

    cout << "Client terminated. Goodbye!" << endl;
    return 0;
}