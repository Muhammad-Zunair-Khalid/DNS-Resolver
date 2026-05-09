#include "user_auth.h"
#include "utils.h"
#include <iostream>
#include <sstream>

using namespace std;

// ==================== User Implementation ====================

User::User() {
    username = "";
    passwordHash = "";
    createdAt = time(nullptr);
    lastLogin = 0;
}

User::User(const string& user, const string& passHash) {
    username = user;
    passwordHash = passHash;
    createdAt = time(nullptr);
    lastLogin = 0;
}

// ==================== UserAuth Implementation ====================

UserAuth::UserAuth() : authMutex()  {
    userTree = new BTree<string, User>(3);
}

UserAuth::~UserAuth() {
    delete userTree;
}

string UserAuth::hashPassword(const string& password) {
    // Simple hash (in production, use bcrypt or SHA-256)
    unsigned long hash = Utils::hashString(password);

    stringstream ss;
    ss << hash;
    return ss.str();
}

string UserAuth::generateSessionID() {
    stringstream ss;
    ss << "SESSION-" << time(nullptr) << "-" << rand() % 10000;
    return ss.str();
}

bool UserAuth::signup(const string& username, const string& password) {
    lock_guard<mutex> lock(authMutex);

    if (username.empty() || password.empty()) {
        cerr << "Username and password cannot be empty" << endl;
        return false;
    }

    if (username.length() < 3 || password.length() < 6) {
        cerr << "Username must be 3+ chars, password must be 6+ chars" << endl;
        return false;
    }

    User existingUser;
    if (userTree->search(username, existingUser)) {
        cerr << "Username already exists" << endl;
        return false;
    }

    string passHash = hashPassword(password);
    User newUser(username, passHash);

    userTree->insert(username, newUser);

    cout << "User registered successfully: " << username << endl;
    return true;
}

string UserAuth::login(const string& username, const string& password) {
    lock_guard<mutex> lock(authMutex);

    User user;
    if (!userTree->search(username, user)) {
        cerr << "User not found" << endl;
        return "";
    }

    string passHash = hashPassword(password);

    if (user.passwordHash != passHash) {
        cerr << "Invalid password" << endl;
        return "";
    }

    // Update last login
    user.lastLogin = time(nullptr);
    userTree->update(username, user);

    // Create session
    string sessionID = generateSessionID();
    activeSessions[sessionID] = username;

    cout << "Login successful: " << username << endl;
    return sessionID;
}

void UserAuth::logout(const string& sessionID) {
    lock_guard<mutex> lock(authMutex);

    auto it = activeSessions.find(sessionID);
    if (it != activeSessions.end()) {
        cout << "User logged out: " << it->second << endl;
        activeSessions.erase(it);
    }
}

bool UserAuth::isAuthenticated(const string& sessionID) {
    lock_guard<mutex> lock(authMutex);

    return activeSessions.find(sessionID) != activeSessions.end();
}

string UserAuth::getUserFromSession(const string& sessionID) {
    lock_guard<mutex> lock(authMutex);

    auto it = activeSessions.find(sessionID);
    if (it != activeSessions.end()) {
        return it->second;
    }

    return "";
}

bool UserAuth::userExists(const string& username) {
    lock_guard<mutex> lock(authMutex);

    User user;
    return userTree->search(username, user);
}

int UserAuth::getTotalUsers() {
    lock_guard<mutex> lock(authMutex);

    return userTree->count();
}

void UserAuth::saveToFile(const string& filename) {
    lock_guard<mutex> lock(authMutex);

    userTree->saveToFile(filename);
    cout << "User database saved to " << filename << endl;
}

void UserAuth::loadFromFile(const string& filename) {
    lock_guard<mutex> lock(authMutex);

    if (Utils::fileExists(filename)) {
        userTree->loadFromFile(filename);
        cout << "User database loaded from " << filename << endl;
    }
}