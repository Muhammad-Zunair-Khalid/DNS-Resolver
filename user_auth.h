#pragma once
#ifndef USER_AUTH_H
#define USER_AUTH_H

#include "btree.h"
#include <string>
#include <map>
#include <mutex>

using namespace std;

struct User {
    string username;
    string passwordHash;
    time_t createdAt;
    time_t lastLogin;

    User();
    User(const string& user, const string& passHash);
};

class UserAuth {
private:
    BTree<string, User>* userTree;
    map<string, string> activeSessions;  // sessionID -> username
    mutex authMutex;

    string hashPassword(const string& password);
    string generateSessionID();

public:
    UserAuth();
    ~UserAuth();

    bool signup(const string& username, const string& password);
    string login(const string& username, const string& password);
    void logout(const string& sessionID);

    bool isAuthenticated(const string& sessionID);
    string getUserFromSession(const string& sessionID);

    bool userExists(const string& username);
    int getTotalUsers();

    void saveToFile(const string& filename);
    void loadFromFile(const string& filename);
};

#endif // USER_AUTH_H