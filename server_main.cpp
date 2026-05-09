#include "dns_server.h"
#include "utils.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

void printMenu() {
    cout << "\n========== DNS SERVER MENU ==========" << endl;
    cout << "1. Add DNS Record" << endl;
    cout << "2. Query Domain" << endl;
    cout << "3. Display Statistics" << endl;
    cout << "4. Display Cache" << endl;
    cout << "5. Clear Cache" << endl;
    cout << "6. Test External DNS" << endl;
    cout << "7. Load Records from File" << endl;
    cout << "8. Display Database" << endl;
    cout << "9. Stop Server" << endl;
    cout << "0. Exit" << endl;
    cout << "=====================================" << endl;
    cout << "Choice: ";
}

int main() {
    cout << "========== DNS RESOLVER SERVER ==========" << endl;
    cout << "Student: Zunair Khalid (BSCS24097)" << endl;
    cout << "========================================\n" << endl;

    // Configure server
    ServerConfig config;
    config.numWorkerThreads = 4;
    config.cacheSize = 1000;
    config.externalDNS = "8.8.8.8";

    // Create server
    DNSServer server(config);

    // Initialize
    if (!server.initialize()) {
        cerr << "Failed to initialize server" << endl;
        return 1;
    }

    // Start server
    if (!server.start()) {
        cerr << "Failed to start server" << endl;
        return 1;
    }

    // Add some default records
    server.addDNSRecord("localhost", "127.0.0.1");
    server.addDNSRecord("example.com", "93.184.216.34");

    // Main menu loop
    bool running = true;

    while (running) {
        printMenu();

        int choice;
        cin >> choice;
        cin.ignore();

        switch (choice) {
        case 1: {
            string domain, ip;
            cout << "Enter domain: ";
            getline(cin, domain);
            cout << "Enter IP: ";
            getline(cin, ip);

            if (server.addDNSRecord(domain, ip)) {
                cout << "Record added successfully" << endl;
            }
            else {
                cout << "Failed to add record" << endl;
            }
            break;
        }

        case 2: {
            string domain;
            cout << "Enter domain to query: ";
            getline(cin, domain);

            DNSRequest request(domain, "127.0.0.1", 5353);
            DNSResponse response = server.processQuery(request);

            cout << "\n" << response.toString() << endl;
            break;
        }

        case 3: {
            server.displayStatistics();
            break;
        }

        case 4: {
            server.displayCache();
            break;
        }

        case 5: {
            server.clearCache();
            break;
        }

        case 6: {
            string domain;
            cout << "Enter domain to test: ";
            getline(cin, domain);

            server.testExternalDNS(domain);
            break;
        }

        case 7: {
            string filename;
            cout << "Enter filename: ";
            getline(cin, filename);

            server.loadRecordsFromFile(filename);
            break;
        }

        case 8: {
            server.displayStatistics();
            break;
        }

        case 9: {
            server.stop();
            cout << "Server stopped" << endl;
            break;
        }

        case 0: {
            running = false;
            break;
        }

        default: {
            cout << "Invalid choice" << endl;
            break;
        }
        }

        this_thread::sleep_for(chrono::milliseconds(500));
    }

    cout << "\nShutting down..." << endl;
    server.stop();

    cout << "Goodbye!" << endl;
    return 0;
}