#ifndef ZIPPY_ROUTER_H
#define ZIPPY_ROUTER_H

#include <string>
#include <algorithm>
#include <vector>
#include <functional>
#include "zippy_request.h"
#include "zippy_utils.h"

typedef std::function<std::string(HTTPRequest &)> RouteFunction;

enum PTNodeType { WORD, VARIABLE, ROOT };

struct PTNode{

    PTNodeType nodeType;
    std::string nodeText;
    RouteFunction stub;
    std::vector<PTNode *> children;

    PTNode * AddChild(std::vector<std::string>::iterator it, const std::vector<std::string>::iterator end_it, RouteFunction stub);
    bool GetStubAndReadURLParameters(std::vector<std::string>::iterator it, const std::vector<std::string>::iterator end_it, RouteFunction & stub, std::map<std::string, Parameter> & param_map);
    PTNode();
    ~PTNode();

    static std::vector<std::string> VectorizePath(const std::string & path_str);
};

class Router{

    private:
    PTNode * path_trie;

    public:
    Router();
    ~Router();
    void AddRoute(const std::string & path, RouteFunction stub);
    bool FindRoute(const std::string & path, RouteFunction &stub, std::map<std::string, Parameter> & param_map);
};

#endif