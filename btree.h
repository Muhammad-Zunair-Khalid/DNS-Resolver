#ifndef BTREE_H
#define BTREE_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>

using namespace std;

// B-Tree Node for hierarchical domain storage
template<typename KeyType, typename ValueType>
class BTreeNode {
private:
    int maxKeys;                          // Maximum keys (2*degree - 1)
    int numKeys;                          // Current number of keys
    bool isLeaf;                          // Leaf node flag

    vector<KeyType> keys;                 // Domain names (reversed: com.google)
    vector<ValueType> values;             // DNS records or user data
    vector<BTreeNode*> children;          // Child pointers
    BTreeNode* parent;                    // Parent pointer

public:
    BTreeNode(int degree, bool leaf);
    ~BTreeNode();

    // Core operations
    void insertNonFull(const KeyType& key, const ValueType& value);
    void splitChild(int index, BTreeNode* child);
    BTreeNode* search(const KeyType& key, ValueType& result);

    // Delete operations
    void remove(const KeyType& key);
    void removeFromLeaf(int index);
    void removeFromNonLeaf(int index);
    KeyType getPredecessor(int index);
    KeyType getSuccessor(int index);
    void fill(int index);
    void borrowFromPrev(int index);
    void borrowFromNext(int index);
    void merge(int index);

    // Traversal
    void traverse(vector<pair<KeyType, ValueType>>& results);

    // Range query helper
    void rangeSearch(const KeyType& startKey, const KeyType& endKey,
        vector<ValueType>& results);

    // Find key index
    int findKey(const KeyType& key);

    // Serialization
    void serialize(ofstream& out);
    static BTreeNode* deserialize(ifstream& in, int degree);

    template<typename K, typename V>
    friend class BTree;
};

// B-Tree for domain hierarchy
template<typename KeyType, typename ValueType>
class BTree {
private:
    BTreeNode<KeyType, ValueType>* root;
    int degree;  // Minimum degree (t)
    int nodeCount;
    int totalKeys;

public:
    BTree(int t = 3);
    ~BTree();

    // Core operations
    void insert(const KeyType& key, const ValueType& value);
    bool search(const KeyType& key, ValueType& result);
    void remove(const KeyType& key);

    // Update existing key
    bool update(const KeyType& key, const ValueType& value);

    // Range query for subdomains
    vector<ValueType> rangeQuery(const KeyType& startKey, const KeyType& endKey);

    // Get all domains
    vector<pair<KeyType, ValueType>> getAllRecords();

    // Display
    void display();
    void displayNode(BTreeNode<KeyType, ValueType>* node, int level);

    // Persistence
    void saveToFile(const string& filename);
    void loadFromFile(const string& filename);

    // Utility
    bool isEmpty() const;
    int count() const;
    int getHeight();
    int getHeightHelper(BTreeNode<KeyType, ValueType>* node);

    // Clear tree
    void clear();
    void clearHelper(BTreeNode<KeyType, ValueType>* node);
};

// ==================== BTreeNode Implementation ====================

template<typename KeyType, typename ValueType>
BTreeNode<KeyType, ValueType>::BTreeNode(int degree, bool leaf) {
    maxKeys = 2 * degree - 1;
    numKeys = 0;
    isLeaf = leaf;
    parent = nullptr;

    keys.resize(maxKeys);
    values.resize(maxKeys);
    children.resize(2 * degree, nullptr);
}

template<typename KeyType, typename ValueType>
BTreeNode<KeyType, ValueType>::~BTreeNode() {
    keys.clear();
    values.clear();
    children.clear();
    parent = nullptr;
}

template<typename KeyType, typename ValueType>
int BTreeNode<KeyType, ValueType>::findKey(const KeyType& key) {
    int index = 0;
    while (index < numKeys && keys[index] < key) {
        index++;
    }
    return index;
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::insertNonFull(const KeyType& key, const ValueType& value) {
    int i = numKeys - 1;

    if (isLeaf) {
        // Find location and shift keys
        while (i >= 0 && keys[i] > key) {
            keys[i + 1] = keys[i];
            values[i + 1] = values[i];
            i--;
        }

        // Insert new key-value
        keys[i + 1] = key;
        values[i + 1] = value;
        numKeys++;
    }
    else {
        // Find child to insert
        while (i >= 0 && keys[i] > key) {
            i--;
        }
        i++;

        // Check if child is full
        if (children[i]->numKeys == maxKeys) {
            splitChild(i, children[i]);

            if (keys[i] < key) {
                i++;
            }
        }
        children[i]->insertNonFull(key, value);
    }
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::splitChild(int index, BTreeNode* child) {
    int degree = (maxKeys + 1) / 2;
    BTreeNode* newNode = new BTreeNode(degree, child->isLeaf);
    newNode->numKeys = degree - 1;

    // Copy second half of keys to new node
    for (int j = 0; j < degree - 1; j++) {
        newNode->keys[j] = child->keys[j + degree];
        newNode->values[j] = child->values[j + degree];
    }

    // Copy second half of children if not leaf
    if (!child->isLeaf) {
        for (int j = 0; j < degree; j++) {
            newNode->children[j] = child->children[j + degree];
        }
    }

    child->numKeys = degree - 1;

    // Shift children of this node
    for (int j = numKeys; j >= index + 1; j--) {
        children[j + 1] = children[j];
    }
    children[index + 1] = newNode;

    // Shift keys of this node
    for (int j = numKeys - 1; j >= index; j--) {
        keys[j + 1] = keys[j];
        values[j + 1] = values[j];
    }

    // Copy middle key up
    keys[index] = child->keys[degree - 1];
    values[index] = child->values[degree - 1];
    numKeys++;
}

template<typename KeyType, typename ValueType>
BTreeNode<KeyType, ValueType>* BTreeNode<KeyType, ValueType>::search(const KeyType& key, ValueType& result) {
    int i = 0;
    while (i < numKeys && key > keys[i]) {
        i++;
    }

    if (i < numKeys && keys[i] == key) {
        result = values[i];
        return this;
    }

    if (isLeaf) {
        return nullptr;
    }

    return children[i]->search(key, result);
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::traverse(vector<pair<KeyType, ValueType>>& results) {
    int i;
    for (i = 0; i < numKeys; i++) {
        if (!isLeaf) {
            children[i]->traverse(results);
        }
        results.push_back(make_pair(keys[i], values[i]));
    }

    if (!isLeaf) {
        children[i]->traverse(results);
    }
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::rangeSearch(const KeyType& startKey, const KeyType& endKey,
    vector<ValueType>& results) {
    int i = 0;

    for (i = 0; i < numKeys; i++) {
        if (!isLeaf) {
            children[i]->rangeSearch(startKey, endKey, results);
        }

        if (keys[i] >= startKey && keys[i] <= endKey) {
            results.push_back(values[i]);
        }
    }

    if (!isLeaf) {
        children[i]->rangeSearch(startKey, endKey, results);
    }
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::remove(const KeyType& key) {
    int index = findKey(key);

    if (index < numKeys && keys[index] == key) {
        if (isLeaf) {
            removeFromLeaf(index);
        }
        else {
            removeFromNonLeaf(index);
        }
    }
    else {
        if (isLeaf) {
            return; // Key not found
        }

        bool isInSubtree = (index == numKeys);

        if (children[index]->numKeys < (maxKeys + 1) / 2) {
            fill(index);
        }

        if (isInSubtree && index > numKeys) {
            children[index - 1]->remove(key);
        }
        else {
            children[index]->remove(key);
        }
    }
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::removeFromLeaf(int index) {
    for (int i = index + 1; i < numKeys; i++) {
        keys[i - 1] = keys[i];
        values[i - 1] = values[i];
    }
    numKeys--;
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::removeFromNonLeaf(int index) {
    KeyType key = keys[index];
    int degree = (maxKeys + 1) / 2;

    if (children[index]->numKeys >= degree) {
        KeyType pred = getPredecessor(index);
        keys[index] = pred;
        children[index]->remove(pred);
    }
    else if (children[index + 1]->numKeys >= degree) {
        KeyType succ = getSuccessor(index);
        keys[index] = succ;
        children[index + 1]->remove(succ);
    }
    else {
        merge(index);
        children[index]->remove(key);
    }
}

template<typename KeyType, typename ValueType>
KeyType BTreeNode<KeyType, ValueType>::getPredecessor(int index) {
    BTreeNode* curr = children[index];
    while (!curr->isLeaf) {
        curr = curr->children[curr->numKeys];
    }
    return curr->keys[curr->numKeys - 1];
}

template<typename KeyType, typename ValueType>
KeyType BTreeNode<KeyType, ValueType>::getSuccessor(int index) {
    BTreeNode* curr = children[index + 1];
    while (!curr->isLeaf) {
        curr = curr->children[0];
    }
    return curr->keys[0];
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::fill(int index) {
    int degree = (maxKeys + 1) / 2;

    if (index != 0 && children[index - 1]->numKeys >= degree) {
        borrowFromPrev(index);
    }
    else if (index != numKeys && children[index + 1]->numKeys >= degree) {
        borrowFromNext(index);
    }
    else {
        if (index != numKeys) {
            merge(index);
        }
        else {
            merge(index - 1);
        }
    }
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::borrowFromPrev(int index) {
    BTreeNode* child = children[index];
    BTreeNode* sibling = children[index - 1];

    for (int i = child->numKeys - 1; i >= 0; i--) {
        child->keys[i + 1] = child->keys[i];
        child->values[i + 1] = child->values[i];
    }

    if (!child->isLeaf) {
        for (int i = child->numKeys; i >= 0; i--) {
            child->children[i + 1] = child->children[i];
        }
    }

    child->keys[0] = keys[index - 1];
    child->values[0] = values[index - 1];

    if (!child->isLeaf) {
        child->children[0] = sibling->children[sibling->numKeys];
    }

    keys[index - 1] = sibling->keys[sibling->numKeys - 1];
    values[index - 1] = sibling->values[sibling->numKeys - 1];

    child->numKeys++;
    sibling->numKeys--;
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::borrowFromNext(int index) {
    BTreeNode* child = children[index];
    BTreeNode* sibling = children[index + 1];

    child->keys[child->numKeys] = keys[index];
    child->values[child->numKeys] = values[index];

    if (!child->isLeaf) {
        child->children[child->numKeys + 1] = sibling->children[0];
    }

    keys[index] = sibling->keys[0];
    values[index] = sibling->values[0];

    for (int i = 1; i < sibling->numKeys; i++) {
        sibling->keys[i - 1] = sibling->keys[i];
        sibling->values[i - 1] = sibling->values[i];
    }

    if (!sibling->isLeaf) {
        for (int i = 1; i <= sibling->numKeys; i++) {
            sibling->children[i - 1] = sibling->children[i];
        }
    }

    child->numKeys++;
    sibling->numKeys--;
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::merge(int index) {
    BTreeNode* child = children[index];
    BTreeNode* sibling = children[index + 1];

    child->keys[child->numKeys] = keys[index];
    child->values[child->numKeys] = values[index];

    for (int i = 0; i < sibling->numKeys; i++) {
        child->keys[child->numKeys + 1 + i] = sibling->keys[i];
        child->values[child->numKeys + 1 + i] = sibling->values[i];
    }

    if (!child->isLeaf) {
        for (int i = 0; i <= sibling->numKeys; i++) {
            child->children[child->numKeys + 1 + i] = sibling->children[i];
        }
    }

    for (int i = index + 1; i < numKeys; i++) {
        keys[i - 1] = keys[i];
        values[i - 1] = values[i];
    }

    for (int i = index + 2; i <= numKeys; i++) {
        children[i - 1] = children[i];
    }

    child->numKeys += sibling->numKeys + 1;
    numKeys--;

    delete sibling;
}

template<typename KeyType, typename ValueType>
void BTreeNode<KeyType, ValueType>::serialize(ofstream& out) {
    out.write(reinterpret_cast<const char*>(&numKeys), sizeof(numKeys));
    out.write(reinterpret_cast<const char*>(&isLeaf), sizeof(isLeaf));

    for (int i = 0; i < numKeys; i++) {
        // Serialize key (string)
        int keyLen = keys[i].length();
        out.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
        out.write(keys[i].c_str(), keyLen);

        // Serialize value (depends on type - handled externally)
        // For now, basic serialization
    }

    if (!isLeaf) {
        for (int i = 0; i <= numKeys; i++) {
            children[i]->serialize(out);
        }
    }
}

template<typename KeyType, typename ValueType>
BTreeNode<KeyType, ValueType>* BTreeNode<KeyType, ValueType>::deserialize(ifstream& in, int degree) {
    int numKeys;
    bool isLeaf;

    in.read(reinterpret_cast<char*>(&numKeys), sizeof(numKeys));
    in.read(reinterpret_cast<char*>(&isLeaf), sizeof(isLeaf));

    BTreeNode* node = new BTreeNode(degree, isLeaf);
    node->numKeys = numKeys;

    for (int i = 0; i < numKeys; i++) {
        int keyLen;
        in.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));

        char* buffer = new char[keyLen + 1];
        in.read(buffer, keyLen);
        buffer[keyLen] = '\0';

        node->keys[i] = string(buffer);
        delete[] buffer;
    }

    if (!isLeaf) {
        for (int i = 0; i <= numKeys; i++) {
            node->children[i] = deserialize(in, degree);
        }
    }

    return node;
}

// ==================== BTree Implementation ====================

template<typename KeyType, typename ValueType>
BTree<KeyType, ValueType>::BTree(int t) {
    degree = t;
    root = nullptr;
    nodeCount = 0;
    totalKeys = 0;
}

template<typename KeyType, typename ValueType>
BTree<KeyType, ValueType>::~BTree() {
    clear();
}

template<typename KeyType, typename ValueType>
void BTree<KeyType, ValueType>::insert(const KeyType& key, const ValueType& value) {
    if (root == nullptr) {
        root = new BTreeNode<KeyType, ValueType>(degree, true);
        root->keys[0] = key;
        root->values[0] = value;
        root->numKeys = 1;
        nodeCount = 1;
        totalKeys = 1;
    }
    else {
        if (root->numKeys == 2 * degree - 1) {
            BTreeNode<KeyType, ValueType>* newRoot = new BTreeNode<KeyType, ValueType>(degree, false);
            newRoot->children[0] = root;
            newRoot->splitChild(0, root);

            int i = 0;
            if (newRoot->keys[0] < key) {
                i++;
            }
            newRoot->children[i]->insertNonFull(key, value);

            root = newRoot;
            nodeCount++;
        }
        else {
            root->insertNonFull(key, value);
        }
        totalKeys++;
    }
}

template<typename KeyType, typename ValueType>
bool BTree<KeyType, ValueType>::search(const KeyType& key, ValueType& result) {
    if (root == nullptr) {
        return false;
    }

    BTreeNode<KeyType, ValueType>* node = root->search(key, result);
    return (node != nullptr);
}

template<typename KeyType, typename ValueType>
void BTree<KeyType, ValueType>::remove(const KeyType& key) {
    if (root == nullptr) {
        return;
    }

    root->remove(key);

    if (root->numKeys == 0) {
        BTreeNode<KeyType, ValueType>* tmp = root;
        if (root->isLeaf) {
            root = nullptr;
        }
        else {
            root = root->children[0];
        }
        delete tmp;
        nodeCount--;
    }
    totalKeys--;
}

template<typename KeyType, typename ValueType>
bool BTree<KeyType, ValueType>::update(const KeyType& key, const ValueType& value) {
    if (root == nullptr) {
        return false;
    }

    ValueType dummy;
    BTreeNode<KeyType, ValueType>* node = root->search(key, dummy);

    if (node != nullptr) {
        // Find key index and update
        int i = 0;
        while (i < node->numKeys && node->keys[i] != key) {
            i++;
        }
        if (i < node->numKeys) {
            node->values[i] = value;
            return true;
        }
    }
    return false;
}

template<typename KeyType, typename ValueType>
vector<ValueType> BTree<KeyType, ValueType>::rangeQuery(const KeyType& startKey, const KeyType& endKey) {
    vector<ValueType> results;
    if (root != nullptr) {
        root->rangeSearch(startKey, endKey, results);
    }
    return results;
}

template<typename KeyType, typename ValueType>
vector<pair<KeyType, ValueType>> BTree<KeyType, ValueType>::getAllRecords() {
    vector<pair<KeyType, ValueType>> results;
    if (root != nullptr) {
        root->traverse(results);
    }
    return results;
}

template<typename KeyType, typename ValueType>
void BTree<KeyType, ValueType>::display() {
    if (root != nullptr) {
        displayNode(root, 0);
    }
    else {
        cout << "Tree is empty" << endl;
    }
}

template<typename KeyType, typename ValueType>
void BTree<KeyType, ValueType>::displayNode(BTreeNode<KeyType, ValueType>* node, int level) {
    if (node != nullptr) {
        cout << "Level " << level << ": ";
        for (int i = 0; i < node->numKeys; i++) {
            cout << node->keys[i] << " ";
        }
        cout << endl;

        if (!node->isLeaf) {
            for (int i = 0; i <= node->numKeys; i++) {
                displayNode(node->children[i], level + 1);
            }
        }
    }
}

template<typename KeyType, typename ValueType>
void BTree<KeyType, ValueType>::saveToFile(const string& filename) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << " for writing" << endl;
        return;
    }

    file.write(reinterpret_cast<const char*>(&degree), sizeof(degree));
    file.write(reinterpret_cast<const char*>(&nodeCount), sizeof(nodeCount));
    file.write(reinterpret_cast<const char*>(&totalKeys), sizeof(totalKeys));

    if (root != nullptr) {
        root->serialize(file);
    }

    file.close();
    cout << "B-Tree saved to " << filename << endl;
}

template<typename KeyType, typename ValueType>
void BTree<KeyType, ValueType>::loadFromFile(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Warning: Could not open file " << filename << " for reading" << endl;
        return;
    }

    clear();

    file.read(reinterpret_cast<char*>(&degree), sizeof(degree));
    file.read(reinterpret_cast<char*>(&nodeCount), sizeof(nodeCount));
    file.read(reinterpret_cast<char*>(&totalKeys), sizeof(totalKeys));

    if (nodeCount > 0) {
        root = BTreeNode<KeyType, ValueType>::deserialize(file, degree);
    }

    file.close();
    cout << "B-Tree loaded from " << filename << endl;
}

template<typename KeyType, typename ValueType>
bool BTree<KeyType, ValueType>::isEmpty() const {
    return root == nullptr;
}

template<typename KeyType, typename ValueType>
int BTree<KeyType, ValueType>::count() const {
    return totalKeys;
}

template<typename KeyType, typename ValueType>
int BTree<KeyType, ValueType>::getHeight() {
    return getHeightHelper(root);
}

template<typename KeyType, typename ValueType>
int BTree<KeyType, ValueType>::getHeightHelper(BTreeNode<KeyType, ValueType>* node) {
    if (node == nullptr) {
        return 0;
    }

    if (node->isLeaf) {
        return 1;
    }

    return 1 + getHeightHelper(node->children[0]);
}

template<typename KeyType, typename ValueType>
void BTree<KeyType, ValueType>::clear() {
    clearHelper(root);
    root = nullptr;
    nodeCount = 0;
    totalKeys = 0;
}

template<typename KeyType, typename ValueType>
void BTree<KeyType, ValueType>::clearHelper(BTreeNode<KeyType, ValueType>* node) {
    if (node != nullptr) {
        if (!node->isLeaf) {
            for (int i = 0; i <= node->numKeys; i++) {
                clearHelper(node->children[i]);
            }
        }
        delete node;
    }
}

#endif // BTREE_H