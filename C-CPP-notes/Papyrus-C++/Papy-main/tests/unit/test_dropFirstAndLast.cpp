#include <string>
#include <iostream>
#include "../../src/matchBuilder.hpp"


/*
Author: Brian Sawa

Date: December 19th 2024

Test Target: matchBuilder.cpp class helper function "dropFirstAndLast()"

Findings: All passed
*/


// Internal function to remove the first and last characters of a string
std::string dropFirstAndLast(const std::string& str) {
    if (str.length() > 2) { return str.substr(1, str.length() - 2); }
    return ""; // Return an empty string if the input is too short
}

class TEST_dropFirstAndLast {
    public:
    static int testAllNumberString() {
        std::cout << "\nTesting an all number string of length 20 \n54353246326575247463" << std::endl;
        std::string testOutput = dropFirstAndLast("54353246326575247463");
        
        std::cout << "Expecting: 435324632657524746" << std::endl;
        std::cout << "Got: " << testOutput << std::endl;
        
        if (testOutput == "435324632657524746") {
            std::cout << "\033[32m==========PASSED==========\033[0m" << std::endl;
        } else {
            std::cout << "\033[31m==========FAILED==========\033[0m" << std::endl;
        }
        return 0;
    }
    static int testAllCharString() {
        std::cout << "\nTesting an all char string of length 11 \nLorem Ipsum" << std::endl;
        std::string testOutput = dropFirstAndLast("Lorem Ipsum");
        std::cout << testOutput << std::endl;

        std::cout << "Expecting: orem Ipsu" << std::endl;
        std::cout << "Got: " << testOutput << std::endl;
        
        if (testOutput == "orem Ipsu") {
            std::cout << "\033[32m==========PASSED==========\033[0m" << std::endl;
        } else {
            std::cout << "\033[31m==========FAILED==========\033[0m" << std::endl;
        }
        return 0;
    }
    static int testAllRandomString() {
        std::cout << "\nTesting an all random string of length 24 \ngfsgege^$ethehe6!)w4htwh" << std::endl;
        std::string testOutput = dropFirstAndLast("gfsgege^$ethehe6!)w4htwh");
        std::cout << testOutput << std::endl;
        
        std::cout << "Expecting: fsgege^$ethehe6!)w4htw" << std::endl;
        std::cout << "Got: " << testOutput << std::endl;
        
        if (testOutput == "fsgege^$ethehe6!)w4htw") {
            std::cout << "\033[32m==========PASSED==========\033[0m" << std::endl;
        } else {
            std::cout << "\033[31m==========FAILED==========\033[0m" << std::endl;
        }
        return 0;
    }
    static int testLengthZeroString() {
        std::cout << "\nTesting an empty string of length 0 \n\"\"" << std::endl;
        std::string testOutput = dropFirstAndLast("");
        std::cout << testOutput << std::endl;

        std::cout << "Expecting: \"\"" << std::endl;
        std::cout << "Got: " << testOutput << std::endl;
        
        if (testOutput == "") {
            std::cout << "\033[32m==========PASSED==========\033[0m" << std::endl;
        } else {
            std::cout << "\033[31m==========FAILED==========\033[0m" << std::endl;
        }
        return 0;
    }
    static int testLengthOneString() {
        std::cout << "\nTesting a string of a single character of length 1 \nB" << std::endl;
        std::string testOutput = dropFirstAndLast("B");
        std::cout << testOutput << std::endl;
        
        std::cout << "Expecting: B" << std::endl;
        std::cout << "Got: " << testOutput << std::endl;
        
        if (testOutput == "") {
            std::cout << "\033[32m==========PASSED==========\033[0m" << std::endl;
        } else {
            std::cout << "\033[31m==========FAILED==========\033[0m" << std::endl;
        }
        return 0;
    }
    static int testLengthTwoString() {
        std::cout << "\nTesting a 2 character string of length 2\nBS" << std::endl;
        std::string testOutput = dropFirstAndLast("BS");
        std::cout << testOutput << std::endl;

        std::cout << "Expecting: BS" << std::endl;
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
    TEST_dropFirstAndLast::testAllNumberString();
    TEST_dropFirstAndLast::testAllCharString();
    TEST_dropFirstAndLast::testAllRandomString();
    TEST_dropFirstAndLast::testLengthOneString();
    TEST_dropFirstAndLast::testLengthTwoString();
    TEST_dropFirstAndLast::testLengthZeroString();

    return 0;
}