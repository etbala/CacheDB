
#pragma once

#include <string>
#include <vector>
#include <utility>
#include <algorithm>

class ZSet {
private:
    // Node structure for tree_by_score
    class ScoreNode {
    public:
        double score;
        std::string member;
        int height;
        ScoreNode* left;
        ScoreNode* right;

        ScoreNode(double s, const std::string& m)
            : score(s), member(m), height(1), left(nullptr), right(nullptr) {}
    };

    // Node structure for tree_by_member
    class MemberNode {
    public:
        std::string member;
        double score;
        int height;
        MemberNode* left;
        MemberNode* right;

        MemberNode(const std::string& m, double s)
            : member(m), score(s), height(1), left(nullptr), right(nullptr) {}
    };

    ScoreNode* tree_by_score;
    MemberNode* tree_by_member;

public:
    ZSet();
    ~ZSet();

    bool zadd(const std::string& member, double score);
    bool zrem(const std::string& member);
    bool zscore(const std::string& member, double& out_score);
    std::vector<std::pair<std::string, double>> zquery(double min_score, const std::string& min_member, int offset, int limit);

private:
    // AVL tree functions for ScoreNode
    int height(ScoreNode* node);
    void updateHeight(ScoreNode* node);
    int balanceFactor(ScoreNode* node);
    ScoreNode* rotateLeft(ScoreNode* x);
    ScoreNode* rotateRight(ScoreNode* y);
    ScoreNode* balance(ScoreNode* node);
    ScoreNode* insert(ScoreNode* node, double score, const std::string& member);
    ScoreNode* remove(ScoreNode* node, double score, const std::string& member);
    void inorder(ScoreNode* node, double min_score, const std::string& min_member, int& offset, int limit, std::vector<std::pair<std::string, double>>& result);
    void destroy(ScoreNode* node);

    // AVL tree functions for MemberNode
    int height(MemberNode* node);
    void updateHeight(MemberNode* node);
    int balanceFactor(MemberNode* node);
    MemberNode* rotateLeft(MemberNode* x);
    MemberNode* rotateRight(MemberNode* y);
    MemberNode* balance(MemberNode* node);
    MemberNode* insert(MemberNode* node, const std::string& member, double score);
    MemberNode* remove(MemberNode* node, const std::string& member);
    MemberNode* find(MemberNode* node, const std::string& member);
    void destroy(MemberNode* node);
};


ZSet::ZSet() : tree_by_score(nullptr), tree_by_member(nullptr) {}

ZSet::~ZSet() {
    destroy(tree_by_score);
    destroy(tree_by_member);
}

int ZSet::height(ScoreNode* node) {
    return node ? node->height : 0;
}

void ZSet::updateHeight(ScoreNode* node) {
    node->height = 1 + std::max(height(node->left), height(node->right));
}

int ZSet::balanceFactor(ScoreNode* node) {
    return height(node->left) - height(node->right);
}

ZSet::ScoreNode* ZSet::rotateLeft(ScoreNode* x) {
    ScoreNode* y = x->right;
    x->right = y->left;
    y->left = x;
    updateHeight(x);
    updateHeight(y);
    return y;
}

ZSet::ScoreNode* ZSet::rotateRight(ScoreNode* y) {
    ScoreNode* x = y->left;
    y->left = x->right;
    x->right = y;
    updateHeight(y);
    updateHeight(x);
    return x;
}

ZSet::ScoreNode* ZSet::balance(ScoreNode* node) {
    updateHeight(node);
    int bf = balanceFactor(node);

    if (bf > 1) {
        if (balanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
        }
        return rotateRight(node);
    }
    if (bf < -1) {
        if (balanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
        }
        return rotateLeft(node);
    }
    return node;
}

ZSet::ScoreNode* ZSet::insert(ScoreNode* node, double score, const std::string& member) {
    if (!node) {
        return new ScoreNode(score, member);
    }
    if (score < node->score || (score == node->score && member < node->member)) {
        node->left = insert(node->left, score, member);
    } else if (score > node->score || (score == node->score && member > node->member)) {
        node->right = insert(node->right, score, member);
    } else {
        // Duplicate node, do nothing
        return node;
    }
    return balance(node);
}

ZSet::ScoreNode* ZSet::remove(ScoreNode* node, double score, const std::string& member) {
    if (!node) {
        return nullptr;
    }
    if (score < node->score || (score == node->score && member < node->member)) {
        node->left = remove(node->left, score, member);
    } else if (score > node->score || (score == node->score && member > node->member)) {
        node->right = remove(node->right, score, member);
    } else {
        // Node found
        if (!node->left) {
            ScoreNode* right = node->right;
            delete node;
            return right;
        } else if (!node->right) {
            ScoreNode* left = node->left;
            delete node;
            return left;
        } else {
            // Two children
            ScoreNode* min = node->right;
            while (min->left) min = min->left;
            node->score = min->score;
            node->member = min->member;
            node->right = remove(node->right, min->score, min->member);
        }
    }
    return balance(node);
}

void ZSet::inorder(ScoreNode* node, double min_score, const std::string& min_member, int& offset, int limit, std::vector<std::pair<std::string, double>>& result) {
    if (!node || (int)result.size() >= limit) return;

    if (node->score > min_score || (node->score == min_score && node->member >= min_member)) {
        inorder(node->left, min_score, min_member, offset, limit, result);
        if (offset > 0) {
            offset--;
        } else if ((int)result.size() < limit) {
            result.emplace_back(node->member, node->score);
        }
        inorder(node->right, min_score, min_member, offset, limit, result);
    } else {
        inorder(node->right, min_score, min_member, offset, limit, result);
    }
}

void ZSet::destroy(ScoreNode* node) {
    if (!node) return;
    destroy(node->left);
    destroy(node->right);
    delete node;
}

int ZSet::height(MemberNode* node) {
    return node ? node->height : 0;
}

void ZSet::updateHeight(MemberNode* node) {
    node->height = 1 + std::max(height(node->left), height(node->right));
}

int ZSet::balanceFactor(MemberNode* node) {
    return height(node->left) - height(node->right);
}

ZSet::MemberNode* ZSet::rotateLeft(MemberNode* x) {
    MemberNode* y = x->right;
    x->right = y->left;
    y->left = x;
    updateHeight(x);
    updateHeight(y);
    return y;
}

ZSet::MemberNode* ZSet::rotateRight(MemberNode* y) {
    MemberNode* x = y->left;
    y->left = x->right;
    x->right = y;
    updateHeight(y);
    updateHeight(x);
    return x;
}

ZSet::MemberNode* ZSet::balance(MemberNode* node) {
    updateHeight(node);
    int bf = balanceFactor(node);

    if (bf > 1) {
        if (balanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
        }
        return rotateRight(node);
    }
    if (bf < -1) {
        if (balanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
        }
        return rotateLeft(node);
    }
    return node;
}

ZSet::MemberNode* ZSet::insert(MemberNode* node, const std::string& member, double score) {
    if (!node) {
        return new MemberNode(member, score);
    }
    if (member < node->member) {
        node->left = insert(node->left, member, score);
    } else if (member > node->member) {
        node->right = insert(node->right, member, score);
    } else {
        // Update score
        node->score = score;
    }
    return balance(node);
}

ZSet::MemberNode* ZSet::remove(MemberNode* node, const std::string& member) {
    if (!node) {
        return nullptr;
    }
    if (member < node->member) {
        node->left = remove(node->left, member);
    } else if (member > node->member) {
        node->right = remove(node->right, member);
    } else {
        // Node found
        if (!node->left) {
            MemberNode* right = node->right;
            delete node;
            return right;
        } else if (!node->right) {
            MemberNode* left = node->left;
            delete node;
            return left;
        } else {
            // Two children
            MemberNode* min = node->right;
            while (min->left) min = min->left;
            node->member = min->member;
            node->score = min->score;
            node->right = remove(node->right, min->member);
        }
    }
    return balance(node);
}

ZSet::MemberNode* ZSet::find(MemberNode* node, const std::string& member) {
    if (!node) return nullptr;
    if (member < node->member) {
        return find(node->left, member);
    } else if (member > node->member) {
        return find(node->right, member);
    } else {
        return node;
    }
}

void ZSet::destroy(MemberNode* node) {
    if (!node) return;
    destroy(node->left);
    destroy(node->right);
    delete node;
}

bool ZSet::zadd(const std::string& member, double score) {
    MemberNode* mnode = find(tree_by_member, member);
    if (mnode) {
        // Member exists, remove from tree_by_score
        tree_by_score = remove(tree_by_score, mnode->score, member);
        // Update member's score
        mnode->score = score;
    } else {
        // Insert into tree_by_member
        tree_by_member = insert(tree_by_member, member, score);
    }
    // Insert into tree_by_score
    tree_by_score = insert(tree_by_score, score, member);
    return true;
}

bool ZSet::zrem(const std::string& member) {
    MemberNode* mnode = find(tree_by_member, member);
    if (!mnode) {
        return false;
    }
    // Remove from tree_by_member
    tree_by_member = remove(tree_by_member, member);
    // Remove from tree_by_score
    tree_by_score = remove(tree_by_score, mnode->score, member);
    return true;
}

bool ZSet::zscore(const std::string& member, double& out_score) {
    MemberNode* mnode = find(tree_by_member, member);
    if (mnode) {
        out_score = mnode->score;
        return true;
    }
    return false;
}

std::vector<std::pair<std::string, double>> ZSet::zquery(double min_score, const std::string& min_member, int offset, int limit) {
    std::vector<std::pair<std::string, double>> result;
    inorder(tree_by_score, min_score, min_member, offset, limit, result);
    return result;
}
