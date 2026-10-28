// ============================================================
// DRILL 3a — SECOND LARGEST ELEMENT (single pass, no sorting)
// COMPILE & RUN:  g++ -std=c++17 03a_second_largest.cpp -o s && ./s
// PREDICTED OUTPUT:  Largest: 35, Second: 34
// ============================================================

#include <iostream>
using namespace std;

int main() {
    int arr[] = {12, 35, 1, 10, 34, 1};
    int n = 6;

    int largest = arr[0];
    int second = -1;                        // -1 = "none yet" (works for positive arrays)

    for (int i = 1; i < n; i++) {
        if (arr[i] > largest) {
            second = largest;               // old champion becomes second
            largest = arr[i];
        } else if (arr[i] > second && arr[i] != largest) {
            second = arr[i];                // != largest skips duplicates of the champion
        }
    }

    cout << "Largest: " << largest << ", Second: " << second << endl;

    // SAY ALOUD if asked about variants:
    // - "Duplicates count as separate ranks" ([5,5,4] -> second is 5):
    //     remove the `arr[i] != largest` condition.
    // - Array may contain negatives: track a bool hasSecond instead of using -1.
    // - Insertions/deletions keep coming: use a sorted structure (set) instead.
    return 0;
}
