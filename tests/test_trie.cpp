#include <gtest/gtest.h>
#include <sstream>
#include <vector>
#include <string>
#include "zippy_routing.h"

TEST(PathTrieTests, PathVectorization){

    std::vector<std::string> hello_world = PTNode::VectorizePath("/hello/world");
    ASSERT_EQ(hello_world[0], "hello");
    ASSERT_EQ(hello_world[1], "world");

    std::vector<std::string> with_urlparams = PTNode::VectorizePath("/need/entity/:id");
    ASSERT_EQ(with_urlparams[0], "need");
    ASSERT_EQ(with_urlparams[1], "entity");
    ASSERT_EQ(with_urlparams[2], ":id");

    std::vector<std::string> mid_urlparams = PTNode::VectorizePath("/feed/:id1/component/:id2");
    ASSERT_EQ(mid_urlparams[0], "feed");
    ASSERT_EQ(mid_urlparams[1], ":id1");
    ASSERT_EQ(mid_urlparams[2], "component");
    ASSERT_EQ(mid_urlparams[3], ":id2");

}

TEST(PathTrieTests, RawTrieCreationTests){

    PTNode * root = new PTNode{};
    root->nodeType = PTNodeType::ROOT;
    root->stub = [](HTTPRequest & request) -> std::string{
        return "Root stub called";
    };

    for(int i = 0; i < 3; ++i){

        std::stringstream name_ss;
        name_ss << "child" << i;

        PTNode * child = new PTNode{};
        child->nodeText = name_ss.str();
        child->nodeType = PTNodeType::WORD;
        child->stub = [i](HTTPRequest &request) -> std::string{

            std::stringstream ss;
            ss << "Child " << i << " stub called.";
            return ss.str();
        };

        root->children.push_back(child);

        for(int j = 0; j < 3; j++){

            name_ss.str("");
            name_ss.clear();
            name_ss << "child" << i << j;

            PTNode * subchild = new PTNode{};
            subchild->nodeText = name_ss.str();
            subchild->nodeType = PTNodeType::WORD;
            subchild->stub = [i, j](HTTPRequest &request) -> std::string{

                std::stringstream ss;
                ss << "Child " << i << "-" << j << " stub called.";
                return ss.str();
            };

            child->children.push_back(subchild);

        }

    }

    std::vector<std::pair<std::string, bool>> test_paths = {
        {"/child1/", true},
        {"/child2/child22/", true},
        {"/child4/", false},
        {"/child0/child06/", false}
    };

    for(auto test_path : test_paths){

        std::vector<std::string> vectorized_path = PTNode::VectorizePath(test_path.first);
        std::map<std::string, Parameter> empty_params;
        RouteFunction rf;
        
        ASSERT_EQ(root->GetStubAndReadURLParameters(vectorized_path.begin(), vectorized_path.end(), rf, empty_params), test_path.second);

    }


    delete root;

}

TEST(PathTrieTests, URLParamHandling){

    PTNode * root = new PTNode{};
    root->nodeType = PTNodeType::ROOT;
    root->stub = [](HTTPRequest & request) -> std::string{
        return "Root stub called";
    };

    for(int i = 0; i < 3; ++i){

        std::stringstream name_ss;
        if(i == 1){
            name_ss << ":";
        }

        name_ss << "child" << i;

        PTNode * child = new PTNode{};
        child->nodeText = name_ss.str();
        child->nodeType = (i == 1) ? PTNodeType::VARIABLE : PTNodeType::WORD;
        child->stub = [i](HTTPRequest &request) -> std::string{

            std::stringstream ss;
            ss << "Child " << i << " stub called.";
            return ss.str();
        };

        root->children.push_back(child);

        for(int j = 0; j < 3; j++){

            name_ss.str("");
            name_ss.clear();

            if(i == 0 && j == 0){
                name_ss << ":";
            }
            name_ss << "child" << i << j;

            PTNode * subchild = new PTNode{};
            subchild->nodeText = name_ss.str();
            subchild->nodeType = (i == 0 && j == 0) ? PTNodeType::VARIABLE : PTNodeType::WORD;
            subchild->stub = [i, j](HTTPRequest &request) -> std::string{

                std::stringstream ss;
                ss << "Child " << i << "-" << j << " stub called.";
                return ss.str();
            };

            child->children.push_back(subchild);

        }
    }

    //Absolutely sauceless, maybe I overdid it here
    std::vector<std::pair<std::string, std::map<std::string, Parameter>>> test_paths = {
        {
            "/child0/Hello/", 
            {
                {"child00", Parameter{"Hello"}}
            }
        },
        {
            "/World/child11/", 
            {
                {"child1", Parameter{"World"}}
            }
        }
    };

    for(auto tp : test_paths){
        std::map<std::string, Parameter> params;
        RouteFunction rf;
        std::vector<std::string> vectorized_path = PTNode::VectorizePath(tp.first);

        bool found = root->GetStubAndReadURLParameters(vectorized_path.begin(), vectorized_path.end(), rf, params);

        ASSERT_EQ(found, true);
        ASSERT_EQ(params == tp.second, true);
    }

    delete root;

}

int main(int argc, char **argv){
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}