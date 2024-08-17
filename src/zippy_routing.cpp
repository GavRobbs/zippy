#include "zippy_routing.h"
#include <iostream>
#include <sstream>

PTNode * PTNode::AddChild(std::vector<std::string>::iterator it, const std::vector<std::string>::iterator end_it, RouteFunction stub)
{
    if(it == end_it){
        return this;
    }

    //Check if the element at the iterator exists as one of the children, and tack it on there
    for(auto child : children){

        if(child->nodeType == PTNodeType::WORD){
            if(child->nodeText == *it){
                return child->AddChild(it + 1, end_it, stub);
            }
        } else if(child->nodeType == PTNodeType::VARIABLE){
            if(it->find(":") == 0){
                //This means its an url parameter
                if(child->nodeText == it->substr(1, it->length())){
                    return child->AddChild(it + 1, end_it, stub);
                }
            }
        }
    }

    //If it reaches here, it doesn't exist, so we create it
    PTNode * newNode = new PTNode{};
    newNode->nodeType = (it->find(":") == 0) ? PTNodeType::VARIABLE : PTNodeType::WORD;
    newNode->nodeText = (newNode->nodeType == PTNodeType::VARIABLE) ? it->substr(1, it->length()) : *it;
    if((it + 1) == end_it){
        newNode->stub = stub;
    }

    children.push_back(newNode);
    return newNode->AddChild(it + 1, end_it, stub);
}


bool PTNode::GetStubAndReadURLParameters(std::vector<std::string>::iterator it, const std::vector<std::string>::iterator end_it, RouteFunction & stub, std::map<std::string, Parameter> & param_map){

    //BASE CASE, return true
    if(it == end_it){
        return true;
    }

    PTNode * next_child_to_process = nullptr;
    bool usedWord = false;
    std::pair<std::string, Parameter> temp_param; 

    //Iterate through the kids
    for(auto child : children){

        if(child->nodeType == PTNodeType::WORD){
            /* Matching a word is a higher priority than matching a variable, so if we get a text match,
            we set the next child to process as this one, and indicate that we've found a matching word
            by setting usedWord = true.*/
            if(child->nodeText == *it){
                next_child_to_process = child;
                usedWord = true;
            } 
        } else if(child->nodeType == PTNodeType::VARIABLE){

            /* Variables are lower priority for matching, so we only match a variable if we haven't found
            a matching word in the node's children. We store the value of that match in temp_param*/

            if(usedWord == false){
                next_child_to_process = child;
                temp_param.first = child->nodeText;
                temp_param.second = Parameter{*it};
            }

        }

    }

    /* If we did find a next child, we set the stub to it. If the child matched a word, then we didn't
    extract an URL parameter in this instance of the recursion. If it did not match a word, we have
    to put the extracted variables in our map and pass it down further in the recursion.
    TODO: Clean up so that it throws an error if two url params have the same name.*/

    if(next_child_to_process != nullptr){
        stub = next_child_to_process->stub;

        if(!usedWord){
            param_map[temp_param.first.substr(1, temp_param.first.length())] = temp_param.second;
        }

        return next_child_to_process->GetStubAndReadURLParameters(it + 1, end_it, stub, param_map);
    }

    /* If you've iterated through the kids and haven't found a match, this is the other base case,
    where we can't find the path, so return false*/

    return false;
}

std::vector<std::string> PTNode::VectorizePath(const std::string & path_str){
    std::istringstream path_stream{path_str};
    std::string component;
    std::vector<std::string> components;

    while(std::getline(path_stream, component, '/')){
        if(component.empty()){
            continue;
        }

        components.push_back(component);
    }

    return components;
}

Router::Router(){
    path_trie = new PTNode{};
    path_trie->nodeType = PTNodeType::ROOT;
    path_trie->nodeText = "";
    path_trie->stub = [](HTTPRequest & request) -> std::string { 
        return ""; 
    };

}

Router::~Router(){

}

PTNode::PTNode(){

}
    
PTNode::~PTNode(){

    for(auto it = children.begin(); it != children.end(); ++it){
        delete *it;
    }

}

void Router::AddRoute(const std::string & path, RouteFunction stub){

    //Set the root path quickly
    if(path == "/"){
        path_trie->stub = stub;
        return;
    }

    std::vector<std::string> components = PTNode::VectorizePath(path);

    path_trie->AddChild(components.begin(), components.end(), stub);
}

bool Router::FindRoute(const std::string & path, RouteFunction &stub, std::map<std::string, Parameter> & param_map){

    if(path == "/"){
        stub = path_trie->stub;
        return true;
    }

    std::vector<std::string> components = PTNode::VectorizePath(path);

    return path_trie->GetStubAndReadURLParameters(components.begin(), components.end(), stub, param_map);
}