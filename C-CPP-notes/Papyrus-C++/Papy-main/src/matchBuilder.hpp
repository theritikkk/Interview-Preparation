#pragma once

// project dependencies
#include "dependencies/json.hpp"

class matchBuilder {
public:
    // Function to generate a random match based on the template
    static nlohmann::json randomMatch();
};
