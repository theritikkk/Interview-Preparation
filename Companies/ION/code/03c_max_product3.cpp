// ============================================================
// DRILL 3c — MAXIMUM PRODUCT OF 3 NUMBERS (the negative trap)
// COMPILE & RUN:  g++ -std=c++17 03c_max_product3.cpp -o p && ./p
// PREDICTED OUTPUT:  Max product of 3: 300
// ============================================================

#include <iostream>
#include <algorithm>      // for sort
using namespace std;

int main() {
    int arr[] = {-10, -10, 1, 3, 2};
    int n = 5;

    sort(arr, arr + n);                       // -10 -10 1 2 3

    long long prod1 = (long long)arr[n-1] * arr[n-2] * arr[n-3];   // 3 largest: 1*2*3 = 6
    long long prod2 = (long long)arr[0] * arr[1] * arr[n-1];       // 2 most-negative x largest:
                                                                   // (-10)(-10)(3) = 300
    if (prod2 > prod1) prod1 = prod2;

    cout << "Max product of 3: " << prod1 << endl;

    // SAY ALOUD: "two big negatives multiply into a big positive —
    // that's why the answer is NOT the three largest numbers."
    return 0;
}
