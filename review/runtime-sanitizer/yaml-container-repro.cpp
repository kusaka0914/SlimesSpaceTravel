#include <iostream>
#include <yaml-cpp/yaml.h>
int main()
{
    YAML::Node node;
    for (int index = 0; index < 20; ++index) {
        node["keep"] = index;
        node["remove"] = index;
        node.remove("remove");
        node["reinsert"] = index;
        node.remove("missing");
        node.remove("reinsert");
    }
    std::cout << node["keep"].as<int>() << '\n';
}
