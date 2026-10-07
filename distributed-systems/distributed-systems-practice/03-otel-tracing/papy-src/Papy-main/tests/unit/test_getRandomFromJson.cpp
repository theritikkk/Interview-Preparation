#include <iostream>
#include <string>
#include <random>
#include "../../src/json.hpp"

using json = nlohmann::json;

// Static helper function to get a random key from a given JSON file
std::string getRandomFromJson(const std::string& jsonString) {

    json items;

    try {
        items = json::parse(jsonString);
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON Parse Error in getRandomFromJson: " << e.what() << std::endl;
        return "";
    }

    // Ensure the JSON is not empty
    if (items.empty()) {
        std::cerr << "Error: items JSON is empty!" << std::endl;
        return "";
    }

    // Collect all keys
    std::vector<std::string> keys;
    for (auto it = items.begin(); it != items.end(); ++it) {
        keys.push_back(it.key());
    }

    // Check if the keys vector is empty
    if (keys.empty()) {
        std::cerr << "Error: No keys available in JSON object." << std::endl;
        return "";
    }

    // Generate a random index
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(0, keys.size() - 1);
    int randomIndex = distrib(gen);

    return keys[randomIndex]; // Return a random key from the file
}












class TEST_getRandomFromJson {
    public:

    static int testValidJsonString () {
        const std::string SECONDARY_RUNES_JSON = R"(
        {
            "8000":"Precision",
            "8100":"Domination",
            "8200":"Sorcery",
            "8300":"Inspiration",
            "8400":"Resolve"
        }
        )";

        std::cout << "\nTesting valid JSON \n" << std::endl;
        std::string testOutput = getRandomFromJson(SECONDARY_RUNES_JSON);
        
        std::cout << "Expecting: 8000 || 8100 || 8200 || 8300 || 8400" << std::endl;
        std::cout << "Got: " << testOutput << std::endl;
        
        if (testOutput == "8000" || testOutput == "8100" || testOutput == "8200" || testOutput == "8300" || testOutput == "8400") {
            std::cout << "\033[32m==========PASSED==========\033[0m" << std::endl;
        } else {
            std::cout << "\033[31m==========FAILED==========\033[0m" << std::endl;
        }

        return 0;
    }

    static int testInvalidJson () {
        
        // Missing the } at the end of the JSON object
        const std::string SECONDARY_RUNES_JSON = R"(
        {
            "8000":"Precision",
            "8100":"Domination",
            "8200":"Sorcery",
            "8300":"Inspiration",
            "8400":"Resolve"
        
        )";

        std::cout << "\nTesting invalid JSON \n" << std::endl;
        std::string testOutput = getRandomFromJson(SECONDARY_RUNES_JSON);
        
        std::cout << "Expecting: \"\"" << std::endl;
        std::cout << "Got: " << testOutput << std::endl;
        
        if (testOutput == "") {
            std::cout << "\033[32m==========PASSED==========\033[0m" << std::endl;
        } else {
            std::cout << "\033[31m==========FAILED==========\033[0m" << std::endl;
        }

        return 0;
    }
    
    static int testInvalidJsonNormalString () {
        const std::string SECONDARY_RUNES_JSON = "BILBO BAGGINS";

        std::cout << "\nTesting invalid JSON. Normal string as input \n" << std::endl;
        std::string testOutput = getRandomFromJson(SECONDARY_RUNES_JSON);
        
        std::cout << "Expecting: Error" << std::endl;
        std::cout << "Got: " << testOutput << std::endl;
        
        if (testOutput == "") {
            std::cout << "\033[32m==========PASSED==========\033[0m" << std::endl;
        } else {
            std::cout << "\033[31m==========FAILED==========\033[0m" << std::endl;
        }

        return 0;
    }

    static int testRandomString () {
        const std::string SECONDARY_RUNES_JSON = "ljekfnbvjkebwvkjewbvebqv";

        std::cout << "\nTesting invalid JSON. Random string as input \n" << std::endl;
        std::string testOutput = getRandomFromJson(SECONDARY_RUNES_JSON);
        
        std::cout << "Expecting: Error" << std::endl;
        std::cout << "Got: " << testOutput << std::endl;
        
        if (testOutput == "") {
            std::cout << "\033[32m==========PASSED==========\033[0m" << std::endl;
        } else {
            std::cout << "\033[31m==========FAILED==========\033[0m" << std::endl;
        }

        return 0;
    }

    static int testEmptyString () {
        const std::string SECONDARY_RUNES_JSON = "";

        std::cout << "\nTesting invalid JSON. Empty string as input \n" << std::endl;
        std::string testOutput = getRandomFromJson(SECONDARY_RUNES_JSON);
        
        std::cout << "Expecting: Error" << std::endl;
        std::cout << "Got: " << testOutput << std::endl;
        
        if (testOutput == "") {
            std::cout << "\033[32m==========PASSED==========\033[0m" << std::endl;
        } else {
            std::cout << "\033[31m==========FAILED==========\033[0m" << std::endl;
        }

        return 0;
    }
};

int main () {
    TEST_getRandomFromJson::testValidJsonString();
    TEST_getRandomFromJson::testInvalidJson();
    TEST_getRandomFromJson::testInvalidJsonNormalString();
    TEST_getRandomFromJson::testRandomString();
    TEST_getRandomFromJson::testEmptyString();
}