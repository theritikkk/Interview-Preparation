// ============================================================
// DRILL 3d — STRING COMPRESSION: "aaabbcccd" -> "a3b2c3d"
// COMPILE & RUN:  g++ -std=c++17 03d_string_compress.cpp -o c && ./c
// PREDICTED OUTPUT:  Compressed: a3b2c3d
// ============================================================

#include <iostream>
#include <string>
using namespace std;

int main() {
    string s = "aaabbcccd";
    string out = "";

    int i = 0;
    while (i < (int)s.size()) {
        int j = i;
        while (j < (int)s.size() && s[j] == s[i]) j++;   // count the run of s[i]

        out += s[i];                                     // the character
        if (j - i > 1) out += to_string(j - i);          // its count (only if > 1)

        i = j;                                           // jump past the run
    }

    cout << "Compressed: " << out << endl;

    // SAY ALOUD:
    // - one pass, O(n); counts >= 10 handled by to_string (multi-digit)
    // - classic rule: if out is LONGER than s, return the original
    return 0;
}
