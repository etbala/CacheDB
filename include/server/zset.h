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


