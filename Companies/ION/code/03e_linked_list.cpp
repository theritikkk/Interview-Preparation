// ============================================================
// DRILL 3e — LINKED LIST: REVERSE + CYCLE DETECTION (Floyd)
// COMPILE & RUN:  g++ -std=c++17 03e_linked_list.cpp -o l && ./l
// PREDICTED OUTPUT:
//   After reverse: 4 3 2 1
//   Cycle? no
//   Cycle now? yes
// ============================================================

#include <iostream>
using namespace std;

struct Node {
    int data;
    Node* next;
    Node(int d) { data = d; next = NULL; }   // simple constructor
};

Node* reverse(Node* head) {
    Node* prev = NULL;
    while (head != NULL) {
        Node* nextNode = head->next;   // 1) save the rest of the list
        head->next = prev;             // 2) flip the arrow backwards
        prev = head;                   // 3) move prev forward
        head = nextNode;               // 4) move head forward
    }
    return prev;                       // prev is the new head
}

bool hasCycle(Node* head) {            // Floyd: slow moves 1, fast moves 2
    Node* slow = head;
    Node* fast = head;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return true; // they met inside the loop -> cycle
    }
    return false;                      // fast fell off the end -> no cycle
}

int main() {
    // build 1 -> 2 -> 3 -> 4
    Node* head = new Node(1);
    head->next = new Node(2);
    head->next->next = new Node(3);
    head->next->next->next = new Node(4);

    head = reverse(head);
    cout << "After reverse: ";
    for (Node* p = head; p != NULL; p = p->next) cout << p->data << " ";
    cout << endl;

    cout << "Cycle? " << (hasCycle(head) ? "yes" : "no") << endl;

    head->next->next->next->next = head;          // create a cycle (1 -> 4 again)
    cout << "Cycle now? " << (hasCycle(head) ? "yes" : "no") << endl;

    // SAY ALOUD:
    // - brute force for cycle: store visited nodes in a set — O(n) extra space.
    //   Floyd: O(n) time, O(1) space.
    // - (demo leaks nodes on purpose; real code frees them — freeing a cyclic
    //   list needs care, break the cycle first.)
    return 0;
}
