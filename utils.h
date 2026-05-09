#pragma once
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <ctime>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

using namespace std;

// Utility Functions
class Utils {
public:
    // String operations
    static string trim(const string& str) {
        string result = str;
        // Trim from start
        size_t start = 0;
        while (start < result.length() && result[start] == ' ') {
            start++;
        }
        // Trim from end
        size_t end = result.length();
        while (end > start && result[end - 1] == ' ') {
            end--;
        }
        return result.substr(start, end - start);
    }

    static vector<string> split(const string& key, char delimiter) {
        string str = key;
        vector<string> result;
        size_t pos = 0;

        while ((pos = str.find(delimiter)) != string::npos) {
            string token = str.substr(0, pos);
            result.push_back(token);
            str.erase(0, pos + 1);
        }

        if (!str.empty()) {
            result.push_back(str);
        }

        return result;
    }

    static string join(const vector<string>& vec, const string& delimiter) {
        if (vec.empty()) {
            return "";
        }
        string result = "";
        for (int i = 0; i < vec.size() - 1; i++) {
            result += vec[i];
            result += delimiter;
        }
        result += vec[vec.size() - 1];
        return result;
    }

    static string toLower(const string& str) {
        string token = "";
        for (char c : str) {
            if (c >= 'A' && c <= 'Z') {
                token += c | 0x20;
            }
            else {
                token += c;
            }
        }
        return token;
    }

    static string toUpper(const string& str) {
        string token = "";
        for (char c : str) {
            if (c >= 'a' && c <= 'z') {
                token += c & 0xDF;
            }
            else {
                token += c;
            }
        }
        return token;
    }

    // Domain name operations
    static string reverseDomain(const string& domain) {
        vector<string> parts = split(domain, '.');
        string result = "";
        for (int i = parts.size() - 1; i >= 0; i--) {
            result += parts[i];
            if (i > 0) result += ".";
        }
        return result;
    }

    static string normalizeDomain(const string& domain) {
        string normalized = toLower(domain);
        normalized = trim(normalized);

        // Remove trailing dot if exists
        if (!normalized.empty() && normalized[normalized.length() - 1] == '.') {
            normalized = normalized.substr(0, normalized.length() - 1);
        }

        return normalized;
    }

    static bool isValidDomain(const string& domain) {
        if (domain.empty() || domain.length() > 253) {
            return false;
        }

        vector<string> labels = split(domain, '.');
        if (labels.empty()) return false;

        for (const string& label : labels) {
            if (label.empty() || label.length() > 63) {
                return false;
            }

            // Check first and last character
            if (label[0] == '-' || label[label.length() - 1] == '-') {
                return false;
            }

            // Check valid characters
            for (char c : label) {
                bool isValid = (c >= 'a' && c <= 'z') ||
                    (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') ||
                    (c == '-');
                if (!isValid) return false;
            }
        }
        return true;
    }

    static string getParentDomain(const string& domain) {
        size_t dotPos = domain.find('.');
        if (dotPos == string::npos) {
            return ""; // No parent
        }
        return domain.substr(dotPos + 1);
    }

    // IP validation
    static bool isValidIPv4(const string& ip) {
        vector<string> result = split(ip, '.');
        if (result.size() != 4) return false;

        for (const string& part : result) {
            if (part.empty()) return false;
            try {
                int a = stoi(part);
                if (a < 0 || a > 255) return false;
            }
            catch (...) {
                return false;
            }
        }
        return true;
    }

    static bool isValidIPv6(const string& ip) {
        vector<string> parts = split(ip, ':');
        if (parts.size() != 8) {
            return false;
        }

        for (const string& part : parts) {
            if (part.empty() || part.size() > 4) {
                return false;
            }

            for (char c : part) {
                bool isHexDigit =
                    (c >= '0' && c <= '9') ||
                    (c >= 'a' && c <= 'f') ||
                    (c >= 'A' && c <= 'F');
                if (!isHexDigit) return false;
            }
        }
        return true;
    }

    // Time operations
    static string formatTimestamp(time_t timestamp) {
#ifdef _WIN32
        struct tm local_tm;
        localtime_s(&local_tm, &timestamp); 
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local_tm);
#else
        struct tm* local_tm = localtime(&timestamp);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local_tm);
#endif
        return string(buffer);
    }

    static time_t getCurrentTime() {
        return time(nullptr);
    }

    static double getTimeDifferenceMs(time_t start, time_t end) {
        return difftime(end, start) * 1000.0;
    }

    // File operations
    static bool fileExists(const string& filename) {
        ifstream file(filename);
        return file.good();
    }

    static bool createDirectory(const string& path) {
        return MKDIR(path.c_str()) == 0;
    }

    static vector<string> readLines(const string& filename) {
        vector<string> lines;
        ifstream file(filename);

        if (!file.is_open()) {
            return lines;
        }

        string line;
        while (getline(file, line)) {
            lines.push_back(line);
        }

        file.close();
        return lines;
    }

    static bool writeLines(const string& filename, const vector<string>& lines) {
        ofstream file(filename);

        if (!file.is_open()) {
            return false;
        }

        for (const string& line : lines) {
            file << line << endl;
        }

        file.close();
        return true;
    }

    // Hash function (simple)
    static unsigned long hashString(const string& str) {
        unsigned long hash = 5381;
        for (char c : str) {
            hash = ((hash << 5) + hash) + c; // hash * 33 + c
        }
        return hash;
    }

    // Random
    static int randomInt(int min, int max) {
        return min + (rand() % (max - min + 1));
    }

    static string generateRequestID() {
        stringstream ss;
        ss << "REQ-" << getCurrentTime() << "-" << randomInt(1000, 9999);
        return ss.str();
    }

    // Display helpers
    static void printSeparator(int length = 60) {
        for (int i = 0; i < length; i++) {
            cout << "=";
        }
        cout << endl;
    }

    static void printHeader(const string& text) {
        printSeparator();
        std::cout << "  " << text << std::endl;  
        printSeparator();
    }

    static string formatBytes(size_t bytes) {
        const char* units[] = { "B", "KB", "MB", "GB", "TB" };
        int unitIndex = 0;
        double size = static_cast<double>(bytes);

        while (size >= 1024.0 && unitIndex < 4) {
            size /= 1024.0;
            unitIndex++;
        }

        stringstream ss;
        ss << fixed << setprecision(2) << size << " " << units[unitIndex];
        return ss.str();
    }
};

#endif // UTILS_H