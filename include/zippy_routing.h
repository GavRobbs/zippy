#ifndef ZIPPY_ROUTER_H
#define ZIPPY_ROUTER_H

#include <string>
#include <algorithm>
#include <vector>

struct HTTPRequest;

typedef std::function<std::string(const HTTPRequest &)> RouteFunction;

enum PTNodeType { WORD, VARIABLE };

struct PTNode{

    PTNodeType nodeType;
    std::string nodeText;
    RouteFunction stub;
    std::vector<PTNode> children;

    PTNode & AddChild(const std::string & ntext, RouteFunction stub);
    int GetStub(const std::string & path, RouteFunction &func);
    
};

class Router{

    private:
    PTNode path_trie;

    public:
    Router();
    ~Router();
    void AddRoute(const std::string & path, RouteFunction stub);
    bool FindRoute(const std::string & path, RouteFunction &stub);
};

#endif