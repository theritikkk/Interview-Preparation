// ============================================================
// DRILL 5 — PRINTING SYSTEM (the dialogue-LLD round — skeleton + tiny demo)
// The interviewer narrates requirements and cross-questions; you add ONE
// stage per answer and NAME the data structure:
//   Stage 1: FIFO order            -> QUEUE          O(1) both ends
//   Stage 2: "VIP job jumps"       -> HEAP (priority) with arrival-time tie-break;
//                                     starvation -> "aging" (wait time raises priority)
//   Stage 3: "double-submit"       -> LRU of recent job IDs (this file demos a simple one)
//   Stage 4: "printer jams"        -> job status QUEUED->PRINTING->DONE/FAILED,
//                                     retry, then dead-letter list + notify
//   Stage 5: "usage per user"      -> ledger  (map<user, pages>)
//   Stage 6: "100 printers"        -> per-printer queues + dispatcher routes by printerId
// COMPILE & RUN:  g++ -std=c++17 05_printing_system.cpp -o pr && ./pr
// PREDICTED OUTPUT:
//   submitted: job 1 (ritik)
//   submitted: job 2 (priya)
//   duplicate rejected: job 1
//   --- printing in FIFO order ---
//   printed job 1 (10 pages)
//   printed job 2 (3 pages)
// ============================================================

#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct PrintJob {
    int id;
    string user;
    int pages;
    int priority;
};

class PrintQueue {                       // STAGE 1: plain FIFO
    vector<PrintJob> jobs;
public:
    void submit(PrintJob j) { jobs.push_back(j); }          // enqueue
    bool empty() { return jobs.empty(); }
    PrintJob serve() {                                      // dequeue from front
        PrintJob j = jobs[0];
        jobs.erase(jobs.begin());      // (a real queue/deque does this in O(1); demo is fine)
        return j;
    }
};

class LRUDedup {                         // STAGE 3: recent-window duplicate check (simple LRU)
    vector<int> recent;                  // most recent FIRST
    int capacity;
public:
    LRUDedup(int cap) { capacity = cap; }
    bool seenRecently(int id) {
        for (int i = 0; i < (int)recent.size(); i++) {
            if (recent[i] == id) {
                recent.erase(recent.begin() + i);          // refresh: it's recent again
                recent.insert(recent.begin(), id);
                return true;
            }
        }
        return false;
    }
    void touch(int id) {                                       // record as most recent
        recent.insert(recent.begin(), id);
        if ((int)recent.size() > capacity) recent.pop_back();  // evict LEAST recent
    }
};

int main() {
    PrintQueue q;
    LRUDedup dedup(3);

    PrintJob a = {1, "ritik", 10, 1};
    PrintJob b = {2, "priya", 3, 2};
    PrintJob dup = {1, "ritik", 10, 1};      // duplicate of a (same job id)

    PrintJob all[] = {a, b, dup};
    for (int i = 0; i < 3; i++) {
        if (dedup.seenRecently(all[i].id)) {
            cout << "duplicate rejected: job " << all[i].id << endl;
        } else {
            dedup.touch(all[i].id);
            q.submit(all[i]);
            cout << "submitted: job " << all[i].id << " (" << all[i].user << ")\n";
        }
    }

    cout << "--- printing in FIFO order ---\n";
    while (!q.empty()) {
        PrintJob j = q.serve();
        cout << "printed job " << j.id << " (" << j.pages << " pages)\n";
    }
    return 0;
}
