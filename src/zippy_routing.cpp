#include "zippy_routing.h"
#include <iostream>

PTNode & PTNode::AddChild(const std::string & ntext, RouteFunction stub)
{
    int slash_index = ntext.find("/");
    std::string target_substr;
    
    if(slash_index == std::string::npos){

        //BASE CASE - the child does not exist
        //so we add it and terminate

        target_substr = ntext;
        PTNode newNode;
        bool isPathParameter = target_substr.find(":") != std::string::npos;
        newNode.nodeType = isPathParameter ? PTNodeType::VARIABLE : PTNodeType::WORD;
        newNode.stub = stub;
        newNode.nodeText = target_substr;
        children.push_back(newNode);
        return children.back();

    } 
    
    //If there are slashes in the path, we cut it at the first slash to get our target substring
    target_substr = ntext.substr(0, slash_index - 1);

    for(auto it = children.begin(); it != children.end(); ++it){

        //Iterate through the children, and if one matches, we pass the path
        //minus the current target substring to it so it works recursively

        if(it->nodeText == target_substr){
            return it->AddChild(ntext.substr(slash_index + 1, ntext.length()), stub);
        }

    }

    //If none of the children match, then we have to add it
    //If it has further path details, then we have to split it up and add it recursively
    std::string remainder_str = ntext.substr(slash_index + 1, ntext.length());

    PTNode newNode;
    newNode.nodeText = target_substr;
    bool isPathParameter = target_substr.find(":") != std::string::npos;
    newNode.nodeType = isPathParameter ? PTNodeType::VARIABLE : PTNodeType::WORD;
    if(remainder_str.length() > 0){
        newNode.AddChild(remainder_str, stub);
    }
    children.push_back(newNode);
    return children.back();    
}

int PTNode::GetStub(const std::string & path, RouteFunction &func)
{
    int slash_index = path.find("/");
    std::string target_substr;
    bool terminal{false};

    if(slash_index == std::string::npos){
        target_substr = path;
        terminal = true;
    } else{
        target_substr = path.substr(0, slash_index - 1);
    }
        
    std::string remainder_string = path.substr(slash_index + 1, path.length());    

    for(auto it = children.begin(); it != children.end(); ++it){

        if(it->nodeText == target_substr){
            //Its a match

            if(terminal){
                func = it->stub;
                return 0;
            } else{
                return it->GetStub(remainder_string, func);
            }
        }
    }

    return -1;

}

Router::Router(){
    path_trie.nodeType = PTNodeType::WORD;
    path_trie.nodeText = "";
    path_trie.stub = [](const HTTPRequest & request) -> std::string { 
        return ""; 
    };

}

Router::~Router(){

}

void Router::AddRoute(const std::string & path, RouteFunction stub){

    //Set the root path quickly
    if(path == "/"){
        path_trie.stub = stub;
        return;
    }

    //Otherwise, remove the leading slash
    std::string path_processed;
    if(path[0] == '/'){
        path_processed = path.substr(1, path.length());
    } else{
        path_processed = path;
    }

    path_trie.AddChild(path_processed, stub);
}

bool Router::FindRoute(const std::string & path, RouteFunction &stub){

    if(path == "/"){
        stub = path_trie.stub;
        return true;
    }

    std::string path_processed;
    if(path[0] == '/'){
        path_processed = path.substr(1, path.length());
    } else{
        path_processed = path;
    }


    int result = path_trie.GetStub(path_processed, stub);
    if(result == 0){
        return true;
    }

    return false;
}