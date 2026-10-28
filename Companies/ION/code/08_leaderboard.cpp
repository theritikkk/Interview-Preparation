// ============================================================
// DRILL 8 — LEADERBOARD (LLD, fresher-level, runnable)
// addScore(player, pts) / top(k) / reset(player)
// Escalations to SAY (he will ask):
//   1. "top(k) called very often?" -> don't sort every time: keep a
//      min-heap of size k, or a sorted set updated on each addScore.
//   2. "Two players same score?" -> tie-break by name (or by who got
//      there first — store a timestamp).
//   3. "Concurrent updates?" -> mutex around addScore (say it).
//   4. "Millions of players?" -> only the top-k matters — heap of size k
//      uses tiny memory; full ranking = sorted structure in a DB.
// COMPILE & RUN:  g++ -std=c++17 08_leaderboard.cpp -o lb && ./lb
// PREDICTED OUTPUT:
//   Top 2:
//     1. Priya - 120
//     2. Ritik - 90
//   After reset:
//     1. Priya - 120
//     2. Ritik - 30
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
using namespace std;

// helper used by sort: score descending
bool byScoreDesc(const pair<string, int>& a, const pair<string, int>& b) {
    if (a.second != b.second) return a.second > b.second;
    return a.first < b.first;              // tie-break: name ascending
}

class Leaderboard {
    map<string, int> scores;               // player -> total score
public:
    void addScore(string player, int pts) { scores[player] += pts; }
    void reset(string player) { scores[player] = 0; }

    void top(int k) {
        // simple version: copy out and sort (O(n log n)) — fine for a demo;
        // SAY the heap/set upgrade for frequent top(k) calls.
        vector<pair<string, int> > v;
        for (map<string, int>::iterator it = scores.begin(); it != scores.end(); ++it)
            v.push_back(make_pair(it->first, it->second));
        sort(v.begin(), v.end(), byScoreDesc);

        cout << "Top " << k << ":" << endl;
        for (int i = 0; i < (int)v.size() && i < k; i++)
            cout << "  " << (i + 1) << ". " << v[i].first << " - " << v[i].second << endl;
    }
};

int main() {
    Leaderboard lb;
    lb.addScore("Ritik", 50);
    lb.addScore("Priya", 120);
    lb.addScore("Ritik", 40);              // Ritik now 90
    lb.top(2);

    lb.reset("Ritik");                     // back to 0
    lb.addScore("Ritik", 30);
    cout << "After reset:" << endl;
    lb.top(2);
    return 0;
}
