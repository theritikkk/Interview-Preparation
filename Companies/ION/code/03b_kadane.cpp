// ============================================================
// DRILL 3b — KADANE: MAX SUBARRAY SUM (+ start and end indices)
// COMPILE & RUN:  g++ -std=c++17 03b_kadane.cpp -o k && ./k
// PREDICTED OUTPUT:  Max sum: 6 from index 3 to 6   (subarray 4,-1,2,1)
// ============================================================

#include <iostream>
using namespace std;

int main() {
    int arr[] = {-2, 1, -3, 4, -1, 2, 1, -5, 4};
    int n = 9;

    int best = arr[0], cur = arr[0];
    int bestStart = 0, curStart = 0, bestEnd = 0;

    for (int i = 1; i < n; i++) {
        if (cur < 0) {              // a negative running sum only hurts -> restart here
            cur = arr[i];
            curStart = i;
        } else {
            cur = cur + arr[i];     // extend the current subarray
        }
        if (cur > best) {           // new best subarray found
            best = cur;
            bestStart = curStart;
            bestEnd = i;
        }
    }

    cout << "Max sum: " << best << " from index " << bestStart << " to " << bestEnd << endl;

    // SAY ALOUD:
    // - All-negative array: this returns the LARGEST SINGLE ELEMENT (best starts at arr[0],
    //   cur always restarts) — say this before they ask.
    // - Modification they may make: "max product subarray" -> carry TWO running values
    //   (max and min ending here), swap them on a negative, reset on zero.
    return 0;
}
