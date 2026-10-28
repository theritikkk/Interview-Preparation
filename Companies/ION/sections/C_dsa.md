# Section C — DSA: Interview-Focused Practice

> **Strategy**: ION interviews are 60 minutes. You won't have time to "discover" solutions. Know these patterns cold — recognize → state approach → code → test edges. All solutions below are **compilable C++17**.

---

## Table of Contents
1. [🔴 ION Must-Solve Problems](#-ion-must-solve-problems)
   - [C.1 Second-Largest Distinct Element](#c1-second-largest-distinct-element-in-an-array)
   - [C.2 Maximum-Sum Subarray (Kadane's)](#c2-maximum-sum-subarray-kadanes-algorithm)
   - [C.3 String Compression](#c3-string-compression)
   - [C.4 Prefix-Search Phone Directory (Trie)](#c4-prefix-search-phone-directory-using-a-trie)
   - [C.5 Maximum Product of Three Elements](#c5-maximum-product-of-three-elements)
   - [C.6 Reverse a Linked List](#c6-reverse-a-linked-list)
   - [C.7 Implement a Linked List](#c7-implement-a-linked-list)
   - [C.8 Detect a Linked-List Cycle](#c8-detect-a-linked-list-cycle-floyds)
   - [C.9 Count Connected Components in a Graph](#c9-count-connected-components-in-a-graph)
   - [C.10 Binary Search + Variations](#c10-binary-search--variations)
   - [C.11 Valid Parentheses](#c11-valid-parentheses)
   - [C.12 Two Sum](#c12-two-sum)
   - [C.13 Top-K Elements](#c13-top-k-elements)
   - [C.14 Job Portal (OA Problem)](#c14-job-portal-oa-problem)
2. [📘 Foundational Topics & Complexity Tables](#-foundational-topics--complexity-tables)
3. [📊 Master Complexity Cheat Sheet](#-master-complexity-cheat-sheet)

---

## 🔴 ION Must-Solve Problems

---

### C.1 Second-Largest Distinct Element in an Array

> **ION frequency: VERY HIGH** — Asked in multiple interview rounds.

#### Clarifying Questions to Ask
- "Are there guaranteed to be at least two distinct elements?"
- "Should I handle empty arrays or arrays with all identical elements?"
- "Are the elements integers? Can they be negative?"

#### Approach 1 — Brute Force O(N²)
Find max, then find max of remaining elements excluding that value.

```cpp
#include <iostream>
#include <vector>
#include <climits>
using namespace std;

int secondLargestBrute(const vector<int>& arr) {
    int n = arr.size();
    if (n < 2) return INT_MIN; // not enough elements

    // Find the maximum
    int maxVal = INT_MIN;
    for (int i = 0; i < n; i++)
        maxVal = max(maxVal, arr[i]);

    // Find the max among elements != maxVal
    int secondMax = INT_MIN;
    for (int i = 0; i < n; i++) {
        if (arr[i] != maxVal)
            secondMax = max(secondMax, arr[i]);
    }
    return secondMax; // INT_MIN if no second distinct exists
}
// Time: O(N) actually two passes — but conceptually "brute"
// Space: O(1)
```

#### Approach 2 — Sorting O(N log N)

```cpp
#include <algorithm>
int secondLargestSort(vector<int> arr) { // copy intentional
    sort(arr.begin(), arr.end(), greater<int>());
    for (int i = 1; i < (int)arr.size(); i++) {
        if (arr[i] != arr[0]) return arr[i];
    }
    return INT_MIN; // no second distinct
}
// Time: O(N log N)   Space: O(1) if in-place sort allowed
```

#### Approach 3 — Optimal Single Pass O(N) ⭐

```cpp
#include <iostream>
#include <vector>
#include <climits>
using namespace std;

int secondLargest(const vector<int>& arr) {
    if (arr.size() < 2) return INT_MIN;

    int first = INT_MIN, second = INT_MIN;
    for (int x : arr) {
        if (x > first) {
            second = first;
            first = x;
        } else if (x > second && x != first) {
            second = x;
        }
    }
    return second; // INT_MIN means no valid answer
}

int main() {
    vector<int> arr = {12, 35, 1, 10, 34, 1};
    int res = secondLargest(arr);
    if (res == INT_MIN) cout << "No second largest\n";
    else cout << "Second largest: " << res << "\n"; // 34
    return 0;
}
// Time: O(N)   Space: O(1)
```

#### Edge Cases to Mention
- All elements identical: `{5, 5, 5}` → no answer
- Two elements: `{3, 7}` → 3
- Negative numbers: `{-1, -5, -3}` → -3
- Single element: return error / INT_MIN

#### Follow-Up: "What if there are updates / a stream?"
> **This is the ION follow-up question!**

- **Stream of elements**: Maintain two variables `first` and `second`, update as each element arrives — O(1) per element.
- **Array with updates (point updates + queries)**: Use a **multiset** or **order-statistics tree** — insert/delete O(log N), second-largest = second from end via `rbegin()`.
- **Segment tree**: Build over the array; each node stores `{first, second}` for its range. Merge two nodes by combining their top-2 values. Query and update both O(log N).

```cpp
// Stream approach — O(1) per new element
struct StreamSecondMax {
    int first = INT_MIN, second = INT_MIN;

    void add(int x) {
        if (x > first) { second = first; first = x; }
        else if (x > second && x != first) second = x;
    }
    int getSecondMax() const { return second; }
};
```

#### How to Explain Aloud
> "I track the two largest distinct values in a single pass. When I find a new maximum, the old maximum becomes the second. If a value is between them and not equal to the first, it becomes the new second. This gives O(N) time and O(1) space."

---

### C.2 Maximum-Sum Subarray (Kadane's Algorithm)

> **ION frequency: HIGH** — Classic DSA question.

#### Clarifying Questions
- "Can the array have all negative numbers?" (Changes the approach slightly.)
- "Do I need to return the subarray indices or just the sum?"
- "Can the array be empty?"

#### Approach 1 — Brute Force O(N²)

```cpp
#include <iostream>
#include <vector>
#include <climits>
using namespace std;

int maxSubarrayBrute(const vector<int>& arr) {
    int n = arr.size();
    int maxSum = INT_MIN;
    for (int i = 0; i < n; i++) {
        int currSum = 0;
        for (int j = i; j < n; j++) {
            currSum += arr[j];
            maxSum = max(maxSum, currSum);
        }
    }
    return maxSum;
}
// Time: O(N²)   Space: O(1)
```

#### Approach 2 — Kadane's Algorithm O(N) ⭐

**Key Insight**: At each position, decide: extend the previous subarray, or start a new one here.

```cpp
#include <iostream>
#include <vector>
#include <climits>
using namespace std;

// Handles all-negative arrays correctly
int maxSubarrayKadane(const vector<int>& arr) {
    int maxSum = INT_MIN;    // global best
    int currSum = 0;          // current subarray sum

    for (int x : arr) {
        currSum += x;
        maxSum = max(maxSum, currSum);
        if (currSum < 0) currSum = 0; // reset — start fresh
    }
    return maxSum;
}

// Version that also tracks subarray boundaries
pair<int, pair<int,int>> maxSubarrayWithIndices(const vector<int>& arr) {
    int maxSum = INT_MIN, currSum = 0;
    int start = 0, end = 0, tempStart = 0;

    for (int i = 0; i < (int)arr.size(); i++) {
        currSum += arr[i];
        if (currSum > maxSum) {
            maxSum = currSum;
            start = tempStart;
            end = i;
        }
        if (currSum < 0) {
            currSum = 0;
            tempStart = i + 1;
        }
    }
    return {maxSum, {start, end}};
}

int main() {
    vector<int> arr = {-2, 1, -3, 4, -1, 2, 1, -5, 4};
    cout << "Max sum: " << maxSubarrayKadane(arr) << "\n"; // 6

    // All negative case
    vector<int> neg = {-3, -1, -5, -2};
    cout << "All neg: " << maxSubarrayKadane(neg) << "\n"; // -1

    auto [sum, indices] = maxSubarrayWithIndices(arr);
    cout << "Sum=" << sum << " from index " << indices.first
         << " to " << indices.second << "\n"; // Sum=6 from index 3 to 6
    return 0;
}
// Time: O(N)   Space: O(1)
```

#### Edge Cases
- All negative: `{-3, -1, -5}` → -1 (single element)
- Single element: `{42}` → 42
- All positive: entire array is the answer
- Contains zeros: zeros can extend a subarray without changing sum

#### How to Explain Aloud
> "Kadane's algorithm maintains a running sum. At each element, I either extend the current subarray or start a new one. If the running sum drops below zero, I reset because any future subarray would be better off starting fresh. This gives O(N) time."

---

### C.3 String Compression

> **ION frequency: HIGH** — Asked to code live. `"aaabbccccd"` → `"a3b2c4d"`

#### Clarifying Questions
- "Should I omit the count if the character appears only once?" → YES based on ION's example (`d` not `d1`)
- "Is the input guaranteed to be lowercase letters only?"
- "Should I modify in-place or return a new string?"
- "What if compressed string is longer than original?" (e.g., `"abcd"` → `"abcd"`)

#### Solution — Two Pointer / Single Pass O(N) ⭐

```cpp
#include <iostream>
#include <string>
using namespace std;

string compress(const string& s) {
    if (s.empty()) return "";

    string result;
    int n = s.size();
    int i = 0;

    while (i < n) {
        char ch = s[i];
        int count = 0;

        // Count consecutive occurrences
        while (i < n && s[i] == ch) {
            count++;
            i++;
        }

        result += ch;
        if (count > 1) {
            result += to_string(count);
        }
    }
    return result;
}

int main() {
    cout << compress("aaabbccccd") << "\n";   // a3b2c4d
    cout << compress("aabbb") << "\n";         // a2b3
    cout << compress("abcd") << "\n";          // abcd (no compression needed)
    cout << compress("a") << "\n";             // a
    cout << compress("aaaa") << "\n";          // a4
    cout << compress("") << "\n";              // (empty)
    return 0;
}
// Time: O(N)   Space: O(N) for the output string
```

#### Edge Cases
- Empty string → return `""`
- Single character → `"a"` → `"a"`
- No consecutive repeats → `"abcd"` → `"abcd"` (same length)
- All same → `"aaaa"` → `"a4"`
- Count > 9 (double digits) → `"aaaaaaaaaaaa"` → `"a12"` — `to_string` handles it

#### How to Explain Aloud
> "I use a two-pointer approach: one pointer tracks the current character, the other counts consecutive duplicates. I append the character and, only if the count exceeds 1, append the count. This runs in O(N) time."

---

### C.4 Prefix-Search Phone Directory Using a Trie

> ⚠️ **ION CRITICAL**: A candidate was **rejected** for suggesting `map` instead of `trie`. Know this cold!

#### Clarifying Questions
- "Should I support partial prefix search — e.g., typing 'an' returns all contacts starting with 'an'?"
- "Are contact names case-sensitive?"
- "Do I need to support deletion?"
- "What's the expected size of the directory?"

#### Why Trie and NOT Map?
| Aspect | `std::map` / `unordered_map` | Trie |
|--------|------------------------------|------|
| Prefix search "an*" | Must iterate ALL keys — O(N×L) | Walk to "an" node, collect below — O(P + K) |
| Autocomplete | Not supported natively | Natural — DFS from prefix node |
| Memory for shared prefixes | Stores full keys redundantly | Shares prefix nodes |
| ION expectation | ❌ **Rejected** | ✅ **Expected** |

Where P = prefix length, K = number of matching results, L = average key length.

#### Complete Trie Implementation ⭐

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
using namespace std;

struct TrieNode {
    TrieNode* children[26] = {};  // a-z
    bool isEnd = false;
    string fullName;              // store the full contact name at end nodes

    ~TrieNode() {
        for (auto& c : children) delete c;
    }
};

class PhoneDirectory {
    TrieNode* root;

    // Collect all words below this node via DFS
    void collectAll(TrieNode* node, vector<string>& results) const {
        if (!node) return;
        if (node->isEnd) {
            results.push_back(node->fullName);
        }
        for (int i = 0; i < 26; i++) {
            collectAll(node->children[i], results);
        }
    }

public:
    PhoneDirectory() : root(new TrieNode()) {}
    ~PhoneDirectory() { delete root; }

    // Insert a contact name — O(L) where L = name length
    void insert(const string& name) {
        TrieNode* curr = root;
        for (char c : name) {
            int idx = tolower(c) - 'a';
            if (idx < 0 || idx >= 26) continue; // skip non-alpha
            if (!curr->children[idx]) {
                curr->children[idx] = new TrieNode();
            }
            curr = curr->children[idx];
        }
        curr->isEnd = true;
        curr->fullName = name;
    }

    // Search by prefix — O(P + K) where P = prefix length, K = results
    vector<string> searchByPrefix(const string& prefix) const {
        TrieNode* curr = root;
        for (char c : prefix) {
            int idx = tolower(c) - 'a';
            if (idx < 0 || idx >= 26 || !curr->children[idx]) {
                return {}; // prefix not in trie
            }
            curr = curr->children[idx];
        }
        vector<string> results;
        collectAll(curr, results);
        return results;
    }

    // Check if exact name exists — O(L)
    bool search(const string& name) const {
        TrieNode* curr = root;
        for (char c : name) {
            int idx = tolower(c) - 'a';
            if (idx < 0 || idx >= 26 || !curr->children[idx]) return false;
            curr = curr->children[idx];
        }
        return curr->isEnd;
    }

    // Delete a contact — O(L)
    bool remove(const string& name) {
        return removeHelper(root, name, 0);
    }

private:
    bool removeHelper(TrieNode* node, const string& name, int depth) {
        if (!node) return false;
        if (depth == (int)name.size()) {
            if (!node->isEnd) return false;
            node->isEnd = false;
            node->fullName.clear();
            // Check if node has any children
            for (auto& c : node->children)
                if (c) return false; // has children, don't delete
            return true; // safe to delete this node
        }
        int idx = tolower(name[depth]) - 'a';
        if (removeHelper(node->children[idx], name, depth + 1)) {
            delete node->children[idx];
            node->children[idx] = nullptr;
            // Delete current if not end and no other children
            if (!node->isEnd) {
                for (auto& c : node->children)
                    if (c) return false;
                return true;
            }
        }
        return false;
    }
};

int main() {
    PhoneDirectory dir;

    // Insert contacts
    dir.insert("ankit");
    dir.insert("ankita");
    dir.insert("anirudh");
    dir.insert("amit");
    dir.insert("bob");
    dir.insert("batman");

    // Simulate typing "an" → show matching contacts
    cout << "Prefix 'an':\n";
    for (const auto& name : dir.searchByPrefix("an"))
        cout << "  " << name << "\n";
    // Output: anirudh, ankit, ankita

    cout << "\nPrefix 'ank':\n";
    for (const auto& name : dir.searchByPrefix("ank"))
        cout << "  " << name << "\n";
    // Output: ankit, ankita

    cout << "\nPrefix 'b':\n";
    for (const auto& name : dir.searchByPrefix("b"))
        cout << "  " << name << "\n";
    // Output: batman, bob

    cout << "\nPrefix 'xyz':\n";
    auto res = dir.searchByPrefix("xyz");
    cout << "  " << (res.empty() ? "(no results)" : "") << "\n";

    cout << "\nSearch 'ankit': " << (dir.search("ankit") ? "found" : "not found") << "\n";
    cout << "Search 'ank': " << (dir.search("ank") ? "found" : "not found") << "\n";

    return 0;
}
```

#### Complexity
| Operation | Time | Space |
|-----------|------|-------|
| Insert | O(L) | O(L) per word (shared prefixes reuse nodes) |
| Prefix Search | O(P + K) | O(K) for results |
| Exact Search | O(L) | O(1) |
| Delete | O(L) | O(L) recursion stack |
| Total Space | — | O(N × L) worst case, much less with shared prefixes |

#### How to Explain Aloud
> "I use a trie because prefix search is its core strength. Each node represents a character, and searching for a prefix means walking down the trie in O(P) time, then collecting all words below via DFS. This is fundamentally more efficient than scanning all entries in a hash map. A map would require checking every key against the prefix — O(N×L) — while the trie is O(P + K) where K is only the matching results."

---

### C.5 Maximum Product of Three Elements

> **ION frequency: MEDIUM-HIGH**

#### Clarifying Questions
- "Can the array contain negative numbers?" → YES, this is the key trick
- "Are there guaranteed to be at least 3 elements?"
- "Can elements be zero?"

#### Key Insight
Two negative numbers multiply to a **positive** number. So the max product could be:
- Three largest positives, OR
- Two smallest negatives × the largest positive

#### Approach 1 — Sort-Based O(N log N)

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int maxProductSort(vector<int> arr) {
    sort(arr.begin(), arr.end());
    int n = arr.size();
    // Two candidates:
    int opt1 = arr[n-1] * arr[n-2] * arr[n-3]; // three largest
    int opt2 = arr[0] * arr[1] * arr[n-1];      // two most-negative × largest
    return max(opt1, opt2);
}
// Time: O(N log N)   Space: O(1) if in-place sort
```

#### Approach 2 — O(N) Linear Scan ⭐

```cpp
#include <iostream>
#include <vector>
#include <climits>
using namespace std;

int maxProductLinear(const vector<int>& arr) {
    // Track 3 largest and 2 smallest
    int max1 = INT_MIN, max2 = INT_MIN, max3 = INT_MIN;
    int min1 = INT_MAX, min2 = INT_MAX;

    for (int x : arr) {
        // Update three largest
        if (x >= max1) {
            max3 = max2; max2 = max1; max1 = x;
        } else if (x >= max2) {
            max3 = max2; max2 = x;
        } else if (x > max3) {
            max3 = x;
        }

        // Update two smallest
        if (x <= min1) {
            min2 = min1; min1 = x;
        } else if (x < min2) {
            min2 = x;
        }
    }
    return max(max1 * max2 * max3, min1 * min2 * max1);
}

int main() {
    cout << maxProductLinear({1, 10, 2, 6, 5, 3}) << "\n";    // 300
    cout << maxProductLinear({-10, -10, 5, 2}) << "\n";         // 500 (two negatives!)
    cout << maxProductLinear({-5, -4, -3, -2, -1}) << "\n";     // -6
    cout << maxProductLinear({0, -1, 3, 100, -70, -50}) << "\n";// 350000
    return 0;
}
// Time: O(N)   Space: O(1)
```

#### Edge Cases
- All negatives: `{-5, -4, -3, -2, -1}` → pick three closest to zero: -1 × -2 × -3 = -6
- Two large negatives + one positive: `{-100, -99, 1, 2, 3}` → 100 × 99 × 3 = 29700
- Contains zeros: `{0, 0, 0, 5}` → 0 (can't form three-element product without enough elements)
- Overflow: mention `long long` if values can be large

#### How to Explain Aloud
> "The trick is that two negatives multiply to a positive, so the maximum product is either the three largest values or the two most-negative values times the largest. I track these five values in a single pass for O(N) time."

---

### C.6 Reverse a Linked List

> **ION frequency: HIGH** — Both iterative and recursive.

#### Clarifying Questions
- "Singly or doubly linked?" → Typically singly
- "In-place or new list?"
- "Do I need to handle empty list or single node?"

#### Iterative ⭐

```cpp
#include <iostream>
using namespace std;

struct ListNode {
    int val;
    ListNode* next;
    ListNode(int v) : val(v), next(nullptr) {}
};

// Iterative — O(N) time, O(1) space
ListNode* reverseIterative(ListNode* head) {
    ListNode* prev = nullptr;
    ListNode* curr = head;
    while (curr) {
        ListNode* nextTemp = curr->next; // save next
        curr->next = prev;               // reverse pointer
        prev = curr;                     // advance prev
        curr = nextTemp;                 // advance curr
    }
    return prev; // new head
}
```

#### Recursive

```cpp
// Recursive — O(N) time, O(N) space (call stack)
ListNode* reverseRecursive(ListNode* head) {
    if (!head || !head->next) return head; // base case

    ListNode* newHead = reverseRecursive(head->next);
    head->next->next = head; // reverse the link
    head->next = nullptr;    // remove old link
    return newHead;
}

// Helper to print
void printList(ListNode* head) {
    while (head) {
        cout << head->val;
        if (head->next) cout << " -> ";
        head = head->next;
    }
    cout << "\n";
}

int main() {
    ListNode* head = new ListNode(1);
    head->next = new ListNode(2);
    head->next->next = new ListNode(3);
    head->next->next->next = new ListNode(4);

    printList(head);                        // 1 -> 2 -> 3 -> 4
    head = reverseIterative(head);
    printList(head);                        // 4 -> 3 -> 2 -> 1
    head = reverseRecursive(head);
    printList(head);                        // 1 -> 2 -> 3 -> 4
    return 0;
}
```

#### How to Explain Aloud
> "Iterative: I use three pointers — prev, curr, next. At each step, I save the next node, point curr back to prev, then advance both pointers. After the loop, prev is the new head. O(N) time, O(1) space."

---

### C.7 Implement a Linked List

> **ION frequency: HIGH** — Insert, delete, search, display.

```cpp
#include <iostream>
using namespace std;

class LinkedList {
    struct Node {
        int data;
        Node* next;
        Node(int val) : data(val), next(nullptr) {}
    };

    Node* head;
    int size;

public:
    LinkedList() : head(nullptr), size(0) {}

    ~LinkedList() {
        Node* curr = head;
        while (curr) {
            Node* temp = curr;
            curr = curr->next;
            delete temp;
        }
    }

    // Insert at front — O(1)
    void insertFront(int val) {
        Node* newNode = new Node(val);
        newNode->next = head;
        head = newNode;
        size++;
    }

    // Insert at end — O(N)
    void insertEnd(int val) {
        Node* newNode = new Node(val);
        if (!head) { head = newNode; size++; return; }
        Node* curr = head;
        while (curr->next) curr = curr->next;
        curr->next = newNode;
        size++;
    }

    // Insert at position (0-indexed) — O(N)
    void insertAt(int pos, int val) {
        if (pos < 0 || pos > size) {
            cout << "Invalid position\n";
            return;
        }
        if (pos == 0) { insertFront(val); return; }
        Node* curr = head;
        for (int i = 0; i < pos - 1; i++) curr = curr->next;
        Node* newNode = new Node(val);
        newNode->next = curr->next;
        curr->next = newNode;
        size++;
    }

    // Delete by value — O(N)
    bool deleteByValue(int val) {
        if (!head) return false;
        if (head->data == val) {
            Node* temp = head;
            head = head->next;
            delete temp;
            size--;
            return true;
        }
        Node* curr = head;
        while (curr->next && curr->next->data != val)
            curr = curr->next;
        if (!curr->next) return false;
        Node* temp = curr->next;
        curr->next = temp->next;
        delete temp;
        size--;
        return true;
    }

    // Delete at position — O(N)
    bool deleteAt(int pos) {
        if (pos < 0 || pos >= size || !head) return false;
        if (pos == 0) {
            Node* temp = head;
            head = head->next;
            delete temp;
            size--;
            return true;
        }
        Node* curr = head;
        for (int i = 0; i < pos - 1; i++) curr = curr->next;
        Node* temp = curr->next;
        curr->next = temp->next;
        delete temp;
        size--;
        return true;
    }

    // Search — O(N)
    bool search(int val) const {
        Node* curr = head;
        while (curr) {
            if (curr->data == val) return true;
            curr = curr->next;
        }
        return false;
    }

    // Display — O(N)
    void display() const {
        Node* curr = head;
        while (curr) {
            cout << curr->data;
            if (curr->next) cout << " -> ";
            curr = curr->next;
        }
        cout << "\n";
    }

    int getSize() const { return size; }
};

int main() {
    LinkedList ll;
    ll.insertEnd(10);
    ll.insertEnd(20);
    ll.insertEnd(30);
    ll.insertFront(5);
    ll.display();            // 5 -> 10 -> 20 -> 30

    ll.insertAt(2, 15);
    ll.display();            // 5 -> 10 -> 15 -> 20 -> 30

    ll.deleteByValue(15);
    ll.display();            // 5 -> 10 -> 20 -> 30

    ll.deleteAt(0);
    ll.display();            // 10 -> 20 -> 30

    cout << "Search 20: " << (ll.search(20) ? "Found" : "Not found") << "\n";
    cout << "Search 99: " << (ll.search(99) ? "Found" : "Not found") << "\n";
    cout << "Size: " << ll.getSize() << "\n";
    return 0;
}
```

#### How to Explain Aloud
> "I maintain a head pointer and size counter. Insert at front is O(1), insert at end or position is O(N). Delete handles edge cases — empty list, deleting head, deleting middle/tail. I use a destructor to prevent memory leaks."

---

### C.8 Detect a Linked-List Cycle (Floyd's)

> **ION frequency: HIGH**

#### Clarifying Questions
- "Should I just detect or also find the start of the cycle?"
- "Can I modify the list?"

```cpp
#include <iostream>
using namespace std;

struct ListNode {
    int val;
    ListNode* next;
    ListNode(int v) : val(v), next(nullptr) {}
};

// Detect cycle — O(N) time, O(1) space ⭐
bool hasCycle(ListNode* head) {
    ListNode* slow = head;
    ListNode* fast = head;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return true;
    }
    return false;
}

// Find the START of the cycle — O(N) time, O(1) space
ListNode* detectCycleStart(ListNode* head) {
    ListNode* slow = head;
    ListNode* fast = head;

    // Phase 1: detect meeting point
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) break;
    }
    if (!fast || !fast->next) return nullptr; // no cycle

    // Phase 2: find cycle start
    // Reset slow to head, move both at speed 1
    slow = head;
    while (slow != fast) {
        slow = slow->next;
        fast = fast->next;
    }
    return slow; // cycle start node
}

int main() {
    // Create: 1 -> 2 -> 3 -> 4 -> 5 -> (back to 3)
    ListNode* head = new ListNode(1);
    head->next = new ListNode(2);
    head->next->next = new ListNode(3);
    head->next->next->next = new ListNode(4);
    head->next->next->next->next = new ListNode(5);
    head->next->next->next->next->next = head->next->next; // cycle to node 3

    cout << "Has cycle: " << (hasCycle(head) ? "Yes" : "No") << "\n"; // Yes
    ListNode* start = detectCycleStart(head);
    if (start) cout << "Cycle starts at: " << start->val << "\n"; // 3
    return 0;
}
// Time: O(N)   Space: O(1)
```

#### Why Floyd's Works (Be Ready to Explain)
- **Phase 1**: Slow moves 1 step, fast moves 2. If there's a cycle, fast will eventually lap slow and they meet inside the cycle. The meeting point is guaranteed because fast gains 1 node per step.
- **Phase 2**: The distance from head to cycle start equals the distance from meeting point to cycle start (going around the cycle). So resetting slow to head and advancing both by 1 makes them meet at the cycle start.

#### How to Explain Aloud
> "Floyd's algorithm uses two pointers — slow moves one step, fast moves two. If they ever meet, there's a cycle. To find the cycle's start, I reset one pointer to head and advance both at speed one — they'll meet at the cycle entry. O(N) time, O(1) space."

---

### C.9 Count Connected Components in a Graph

> **ION frequency: MEDIUM**

#### Clarifying Questions
- "Is the graph directed or undirected?"
- "Is it given as adjacency list, matrix, or edge list?"
- "Can there be self-loops?"

```cpp
#include <iostream>
#include <vector>
#include <queue>
using namespace std;

class Graph {
    int V;
    vector<vector<int>> adj;

    void dfs(int node, vector<bool>& visited) {
        visited[node] = true;
        for (int neighbor : adj[node]) {
            if (!visited[neighbor]) {
                dfs(neighbor, visited);
            }
        }
    }

    void bfs(int start, vector<bool>& visited) {
        queue<int> q;
        q.push(start);
        visited[start] = true;
        while (!q.empty()) {
            int node = q.front(); q.pop();
            for (int neighbor : adj[node]) {
                if (!visited[neighbor]) {
                    visited[neighbor] = true;
                    q.push(neighbor);
                }
            }
        }
    }

public:
    Graph(int v) : V(v), adj(v) {}

    void addEdge(int u, int v) {
        adj[u].push_back(v);
        adj[v].push_back(u); // undirected
    }

    // Count connected components using DFS
    int countComponentsDFS() {
        vector<bool> visited(V, false);
        int count = 0;
        for (int i = 0; i < V; i++) {
            if (!visited[i]) {
                dfs(i, visited);
                count++;
            }
        }
        return count;
    }

    // Count connected components using BFS
    int countComponentsBFS() {
        vector<bool> visited(V, false);
        int count = 0;
        for (int i = 0; i < V; i++) {
            if (!visited[i]) {
                bfs(i, visited);
                count++;
            }
        }
        return count;
    }

    // Union-Find approach — O(N α(N)) ≈ O(N)
    // Better for dynamic edge additions
};

// Union-Find (Disjoint Set Union) alternative
class DSU {
    vector<int> parent, rank_;
public:
    DSU(int n) : parent(n), rank_(n, 0) {
        for (int i = 0; i < n; i++) parent[i] = i;
    }
    int find(int x) {
        if (parent[x] != x) parent[x] = find(parent[x]); // path compression
        return parent[x];
    }
    bool unite(int x, int y) {
        int px = find(x), py = find(y);
        if (px == py) return false;
        if (rank_[px] < rank_[py]) swap(px, py);
        parent[py] = px;
        if (rank_[px] == rank_[py]) rank_[px]++;
        return true;
    }
    int countComponents(int n) {
        int count = 0;
        for (int i = 0; i < n; i++)
            if (find(i) == i) count++;
        return count;
    }
};

int main() {
    //  0 - 1    2 - 3    4   (three components)
    Graph g(5);
    g.addEdge(0, 1);
    g.addEdge(2, 3);

    cout << "Components (DFS): " << g.countComponentsDFS() << "\n"; // 3
    cout << "Components (BFS): " << g.countComponentsBFS() << "\n"; // 3

    // Using DSU
    DSU dsu(5);
    dsu.unite(0, 1);
    dsu.unite(2, 3);
    cout << "Components (DSU): " << dsu.countComponents(5) << "\n"; // 3

    return 0;
}
// DFS/BFS: Time O(V+E), Space O(V)
// DSU:     Time O(V α(V)), Space O(V)
```

#### How to Explain Aloud
> "I iterate over all nodes. For each unvisited node, I start a DFS/BFS that marks its entire connected component as visited, then increment the component count. Time is O(V+E) since we visit every node and edge once."

---

### C.10 Binary Search + Variations

> **ION frequency: MEDIUM**

```cpp
#include <iostream>
#include <vector>
using namespace std;

// Standard binary search — O(log N)
int binarySearch(const vector<int>& arr, int target) {
    int lo = 0, hi = arr.size() - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2; // prevents overflow
        if (arr[mid] == target) return mid;
        else if (arr[mid] < target) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1; // not found
}

// First occurrence of target — O(log N)
int firstOccurrence(const vector<int>& arr, int target) {
    int lo = 0, hi = arr.size() - 1, ans = -1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (arr[mid] == target) {
            ans = mid;
            hi = mid - 1; // keep searching left
        } else if (arr[mid] < target) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return ans;
}

// Last occurrence of target — O(log N)
int lastOccurrence(const vector<int>& arr, int target) {
    int lo = 0, hi = arr.size() - 1, ans = -1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (arr[mid] == target) {
            ans = mid;
            lo = mid + 1; // keep searching right
        } else if (arr[mid] < target) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return ans;
}

// Count occurrences = last - first + 1
int countOccurrences(const vector<int>& arr, int target) {
    int first = firstOccurrence(arr, target);
    if (first == -1) return 0;
    return lastOccurrence(arr, target) - first + 1;
}

int main() {
    vector<int> arr = {1, 2, 3, 3, 3, 4, 5, 6};
    cout << "Search 3: index " << binarySearch(arr, 3) << "\n";
    cout << "First 3: index " << firstOccurrence(arr, 3) << "\n";   // 2
    cout << "Last 3: index " << lastOccurrence(arr, 3) << "\n";     // 4
    cout << "Count 3: " << countOccurrences(arr, 3) << "\n";        // 3
    cout << "Search 7: index " << binarySearch(arr, 7) << "\n";     // -1
    return 0;
}
```

#### Binary Search Template (Memorize This)
```
int lo = 0, hi = n - 1;
while (lo <= hi) {
    int mid = lo + (hi - lo) / 2;
    if (check(mid)) {
        ans = mid;
        hi = mid - 1;  // or lo = mid + 1 depending on which bound
    } else {
        lo = mid + 1;  // or hi = mid - 1
    }
}
```

---

### C.11 Valid Parentheses

> **ION frequency: MEDIUM**

```cpp
#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>
using namespace std;

bool isValid(const string& s) {
    stack<char> st;
    unordered_map<char, char> match = {
        {')', '('}, {']', '['}, {'}', '{'}
    };

    for (char c : s) {
        if (c == '(' || c == '[' || c == '{') {
            st.push(c);
        } else {
            if (st.empty() || st.top() != match[c]) return false;
            st.pop();
        }
    }
    return st.empty(); // must be empty — no unmatched opens
}

int main() {
    cout << isValid("()[]{}") << "\n";   // 1 (true)
    cout << isValid("(]") << "\n";       // 0 (false)
    cout << isValid("([{}])") << "\n";   // 1 (true)
    cout << isValid("((()") << "\n";     // 0 (false)
    cout << isValid("") << "\n";         // 1 (true — empty is valid)
    return 0;
}
// Time: O(N)   Space: O(N)
```

#### Edge Cases
- Empty string → valid
- Only opening brackets → invalid (stack not empty)
- Only closing brackets → invalid (stack empty when popping)
- Single character → invalid

#### How to Explain Aloud
> "I use a stack. Push opening brackets, and for each closing bracket, check if the stack's top matches. If not, or if the stack is empty, it's invalid. At the end, the stack must be empty."

---

### C.12 Two Sum

> **ION frequency: MEDIUM**

```cpp
#include <iostream>
#include <vector>
#include <unordered_map>
using namespace std;

// Brute force — O(N²)
vector<int> twoSumBrute(const vector<int>& nums, int target) {
    for (int i = 0; i < (int)nums.size(); i++)
        for (int j = i + 1; j < (int)nums.size(); j++)
            if (nums[i] + nums[j] == target) return {i, j};
    return {};
}

// Hash map — O(N) ⭐
vector<int> twoSum(const vector<int>& nums, int target) {
    unordered_map<int, int> seen; // value → index
    for (int i = 0; i < (int)nums.size(); i++) {
        int complement = target - nums[i];
        if (seen.count(complement)) {
            return {seen[complement], i};
        }
        seen[nums[i]] = i;
    }
    return {}; // no solution
}

int main() {
    auto res = twoSum({2, 7, 11, 15}, 9);
    cout << "[" << res[0] << ", " << res[1] << "]\n"; // [0, 1]

    res = twoSum({3, 2, 4}, 6);
    cout << "[" << res[0] << ", " << res[1] << "]\n"; // [1, 2]
    return 0;
}
// Time: O(N)   Space: O(N)
```

#### Follow-Up: Sorted Array → Two Pointers O(1) Space
```cpp
vector<int> twoSumSorted(const vector<int>& nums, int target) {
    int lo = 0, hi = nums.size() - 1;
    while (lo < hi) {
        int sum = nums[lo] + nums[hi];
        if (sum == target) return {lo, hi};
        else if (sum < target) lo++;
        else hi--;
    }
    return {};
}
```

#### How to Explain Aloud
> "For each element, I compute the complement needed to reach the target and check if I've already seen it using a hash map. This gives O(N) time with a single pass. If the array is sorted, I can use two pointers for O(1) extra space."

---

### C.13 Top-K Elements

> **ION frequency: MEDIUM**

#### Clarifying Questions
- "Top K largest or smallest?"
- "Can there be duplicates?"
- "Do I need them sorted?"

```cpp
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
using namespace std;

// Approach 1: Sort — O(N log N)
vector<int> topKSort(vector<int> nums, int k) {
    sort(nums.begin(), nums.end(), greater<int>());
    return vector<int>(nums.begin(), nums.begin() + k);
}

// Approach 2: Min-Heap of size K — O(N log K) ⭐
vector<int> topKHeap(const vector<int>& nums, int k) {
    // Min-heap keeps the K largest seen so far
    priority_queue<int, vector<int>, greater<int>> minHeap;

    for (int x : nums) {
        minHeap.push(x);
        if ((int)minHeap.size() > k) {
            minHeap.pop(); // remove smallest — not in top K
        }
    }

    vector<int> result;
    while (!minHeap.empty()) {
        result.push_back(minHeap.top());
        minHeap.pop();
    }
    return result; // smallest to largest of top K
}

// Approach 3: Quickselect — O(N) average (O(N²) worst)
// nth_element in C++ uses this internally
vector<int> topKQuickselect(vector<int> nums, int k) {
    nth_element(nums.begin(), nums.begin() + k - 1, nums.end(), greater<int>());
    return vector<int>(nums.begin(), nums.begin() + k); // not sorted
}

int main() {
    vector<int> nums = {3, 1, 5, 12, 2, 11, 7};
    int k = 3;

    cout << "Top " << k << " (heap): ";
    for (int x : topKHeap(nums, k)) cout << x << " ";
    cout << "\n"; // 7 11 12 (or similar order)

    cout << "Top " << k << " (quickselect): ";
    for (int x : topKQuickselect(nums, k)) cout << x << " ";
    cout << "\n";
    return 0;
}
```

#### When to Use What
| Method | Time | Space | Best For |
|--------|------|-------|----------|
| Sort | O(N log N) | O(1) | Small N, need sorted output |
| Min-Heap of size K | O(N log K) | O(K) | **Streaming data**, K << N |
| Quickselect | O(N) avg | O(1) | When you don't need sorted output |

#### How to Explain Aloud
> "I use a min-heap of size K. As I scan the array, I push each element and pop the minimum whenever the heap exceeds size K. After processing all elements, the heap contains exactly the top K largest. Time is O(N log K), which beats sorting when K is much smaller than N."

---

### C.14 Job Portal (OA Problem)

> **ION frequency: ASKED IN OA** — `postJob`, `postUser`, `fetchJobs(userId, k)`

#### Problem Understanding
- **postJob(jobId, skills)**: Add a job requiring certain skills
- **postUser(userId, skills)**: Register a user with certain skills
- **fetchJobs(userId, k)**: Return top-K jobs for the user, ranked by number of matching skills

```cpp
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
using namespace std;

class JobPortal {
    // jobId → set of required skills
    unordered_map<string, unordered_set<string>> jobs;
    // userId → set of user skills
    unordered_map<string, unordered_set<string>> users;

public:
    void postJob(const string& jobId, const vector<string>& skills) {
        jobs[jobId] = unordered_set<string>(skills.begin(), skills.end());
    }

    void postUser(const string& userId, const vector<string>& skills) {
        users[userId] = unordered_set<string>(skills.begin(), skills.end());
    }

    // Return top-K job IDs ranked by skill match count (descending)
    vector<string> fetchJobs(const string& userId, int k) {
        if (users.find(userId) == users.end()) return {};
        const auto& userSkills = users[userId];

        // Min-heap: (matchCount, jobId) — keep top K
        // Use min-heap so we can pop the least relevant
        using P = pair<int, string>;
        priority_queue<P, vector<P>, greater<P>> minHeap;

        for (const auto& [jobId, jobSkills] : jobs) {
            int matchCount = 0;
            for (const auto& skill : userSkills) {
                if (jobSkills.count(skill)) matchCount++;
            }
            if (matchCount > 0) { // only consider if at least 1 skill matches
                minHeap.push({matchCount, jobId});
                if ((int)minHeap.size() > k) {
                    minHeap.pop();
                }
            }
        }

        // Extract results (will be in ascending order of match)
        vector<string> result;
        while (!minHeap.empty()) {
            result.push_back(minHeap.top().second);
            minHeap.pop();
        }
        reverse(result.begin(), result.end()); // best match first
        return result;
    }
};

int main() {
    JobPortal portal;

    portal.postJob("J1", {"C++", "Python", "SQL", "Docker"});
    portal.postJob("J2", {"Java", "Spring", "SQL"});
    portal.postJob("J3", {"C++", "Docker", "Kubernetes", "AWS"});
    portal.postJob("J4", {"Python", "ML", "TensorFlow"});
    portal.postJob("J5", {"C++", "SQL", "Redis"});

    portal.postUser("U1", {"C++", "SQL", "Docker", "Redis"});

    cout << "Top 3 jobs for U1:\n";
    for (const auto& jobId : portal.fetchJobs("U1", 3)) {
        cout << "  " << jobId << "\n";
    }
    // Expected: J1 (3 matches: C++, SQL, Docker),
    //           J3 (2 matches: C++, Docker),
    //           J5 (2 matches: C++, SQL, Redis => 3 matches!)
    //   Actual order depends on match counts

    return 0;
}
```

#### Complexity
| Operation | Time | Space |
|-----------|------|-------|
| postJob | O(S) where S = skills count | O(S) |
| postUser | O(S) | O(S) |
| fetchJobs | O(J × S_u × log K) | O(K) for heap |

Where J = number of jobs, S_u = user skill count, K = requested count.

#### Optimization Ideas (Mention if Asked)
- **Inverted index**: Map each skill → list of job IDs. For a user, union the job lists for their skills with frequency counting. Avoids scanning ALL jobs.
- **Caching**: If users query frequently, cache results and invalidate on new job posts.

#### How to Explain Aloud
> "I store jobs and users with their skill sets. For fetchJobs, I compute the skill overlap between the user and each job, then use a min-heap of size K to efficiently track the top-K best-matching jobs. For scale, I'd add an inverted index from skills to jobs."

---

## 📘 Foundational Topics & Complexity Tables

### Arrays & Strings — Common Patterns

| Pattern | When to Use | Example Problem |
|---------|-------------|-----------------|
| Two pointers | Sorted array, pair finding | Two Sum (sorted), Container with most water |
| Sliding window | Subarray/substring of fixed or variable size | Max sum of size K, Longest substring without repeats |
| Prefix sum | Range sum queries | Subarray sum equals K |
| Hash map | Frequency counting, lookups | Two Sum, Anagram check |
| Sorting + scan | Need ordered processing | Merge intervals, 3Sum |
| Kadane's | Max/min subarray | Max subarray sum |

### Hash Maps & Sets

```
unordered_map<K,V> map;   // O(1) avg insert/lookup/delete
unordered_set<K> set;     // O(1) avg insert/lookup/delete
map<K,V> ordered_map;     // O(log N) — use when order matters
```

| Operation | `unordered_map` (avg) | `unordered_map` (worst) | `map` (BST) |
|-----------|----------------------|------------------------|-------------|
| Insert | O(1) | O(N) | O(log N) |
| Lookup | O(1) | O(N) | O(log N) |
| Delete | O(1) | O(N) | O(log N) |

**When worst case matters** (e.g., adversarial input): Use `map` (balanced BST) for guaranteed O(log N).

### Linked Lists — Types & Usage

| Type | Key Feature | Use Case |
|------|-------------|----------|
| Singly | next pointer only | Simple stacks, queues, basic lists |
| Doubly | next + prev pointers | LRU cache, deque, browser history |
| Circular | Last node points to first | Round-robin scheduling, circular buffer |

### Stacks & Queues — Applications

| Data Structure | Key Operations | Applications |
|---------------|----------------|-------------|
| Stack (LIFO) | push, pop, top — O(1) | Parentheses matching, undo, DFS, monotonic stack |
| Queue (FIFO) | push, pop, front — O(1) | BFS, task scheduling, sliding window (deque) |
| Deque | push/pop both ends — O(1) | Sliding window max/min, palindrome check |
| Priority Queue | push O(log N), top O(1), pop O(log N) | Dijkstra's, Top-K, merge K sorted lists |

### Trees, Heaps & Priority Queues

**BST Operations:**
| Operation | Average | Worst (unbalanced) | Balanced (AVL/Red-Black) |
|-----------|---------|--------------------|-----------------------|
| Search | O(log N) | O(N) | O(log N) |
| Insert | O(log N) | O(N) | O(log N) |
| Delete | O(log N) | O(N) | O(log N) |

**Heap / Priority Queue:**
| Operation | Time |
|-----------|------|
| Insert | O(log N) |
| Extract min/max | O(log N) |
| Peek min/max | O(1) |
| Build heap | O(N) |
| Heapify | O(log N) |

```cpp
// Min-heap (default in C++ is max-heap)
priority_queue<int, vector<int>, greater<int>> minHeap;

// Max-heap (default)
priority_queue<int> maxHeap;

// Custom comparator for pairs (sort by second value)
auto cmp = [](const pair<int,int>& a, const pair<int,int>& b) {
    return a.second > b.second; // min-heap by second
};
priority_queue<pair<int,int>, vector<pair<int,int>>, decltype(cmp)> pq(cmp);
```

### Tries — Prefix Search & Autocomplete

**When to use a Trie (not a map!):**
- Prefix search / autocomplete → Trie ✅
- Exact key lookup only → Hash map ✅
- Ordered iteration → `std::map` ✅

| Operation | Time |
|-----------|------|
| Insert word | O(L) where L = word length |
| Search exact | O(L) |
| Search prefix | O(P + K) where P = prefix length, K = results |
| Space | O(ALPHABET_SIZE × N × L) worst case |

### Graphs — BFS vs DFS

| Aspect | BFS | DFS |
|--------|-----|-----|
| Data Structure | Queue | Stack / recursion |
| Order | Level by level | Go deep first |
| Shortest path (unweighted) | ✅ Yes | ❌ No |
| Cycle detection (directed) | ❌ Use DFS | ✅ Use colors / back edge |
| Cycle detection (undirected) | ✅ Yes | ✅ Yes |
| Connected components | ✅ | ✅ |
| Topological sort | ✅ Kahn's | ✅ DFS + reverse finish |
| Time complexity | O(V + E) | O(V + E) |
| Space complexity | O(V) | O(V) |

**Graph Representation:**
| Representation | Space | Edge lookup | Add edge | Best for |
|----------------|-------|-------------|----------|----------|
| Adjacency list | O(V + E) | O(degree) | O(1) | Sparse graphs |
| Adjacency matrix | O(V²) | O(1) | O(1) | Dense graphs |
| Edge list | O(E) | O(E) | O(1) | Kruskal's MST |

### Sorting — Comparison

| Algorithm | Best | Average | Worst | Space | Stable | Notes |
|-----------|------|---------|-------|-------|--------|-------|
| Bubble Sort | O(N) | O(N²) | O(N²) | O(1) | ✅ | Only for teaching |
| Selection Sort | O(N²) | O(N²) | O(N²) | O(1) | ❌ | Minimum swaps |
| Insertion Sort | O(N) | O(N²) | O(N²) | O(1) | ✅ | Best for nearly sorted |
| Merge Sort | O(N log N) | O(N log N) | O(N log N) | O(N) | ✅ | Guaranteed performance |
| Quick Sort | O(N log N) | O(N log N) | O(N²) | O(log N) | ❌ | Fastest in practice |
| Heap Sort | O(N log N) | O(N log N) | O(N log N) | O(1) | ❌ | In-place, not cache-friendly |
| Counting Sort | O(N + K) | O(N + K) | O(N + K) | O(K) | ✅ | K = range of values |
| Radix Sort | O(d × (N+K)) | O(d × (N+K)) | O(d × (N+K)) | O(N + K) | ✅ | d = digits |

> **C++ `std::sort`** uses **IntroSort** (QuickSort + HeapSort + InsertionSort) → O(N log N) guaranteed.

### Two Pointers & Sliding Window

```cpp
// Two pointers template (sorted array, find pair with target sum)
int lo = 0, hi = n - 1;
while (lo < hi) {
    int sum = arr[lo] + arr[hi];
    if (sum == target) { /* found */ lo++; hi--; }
    else if (sum < target) lo++;
    else hi--;
}

// Sliding window template (variable size, find min window with condition)
int lo = 0;
for (int hi = 0; hi < n; hi++) {
    // expand window: add arr[hi]
    while (/* window satisfies condition */) {
        // update answer
        // shrink window: remove arr[lo]
        lo++;
    }
}

// Fixed-size sliding window
int windowSum = 0;
for (int i = 0; i < n; i++) {
    windowSum += arr[i];
    if (i >= k) windowSum -= arr[i - k]; // remove leftmost
    if (i >= k - 1) maxSum = max(maxSum, windowSum);
}
```

### Prefix Sums

```cpp
// Build prefix sum
vector<int> prefix(n + 1, 0);
for (int i = 0; i < n; i++)
    prefix[i + 1] = prefix[i] + arr[i];

// Query range sum [l, r] (0-indexed) in O(1)
int rangeSum = prefix[r + 1] - prefix[l];

// Application: count subarrays with sum = target
unordered_map<int, int> prefixCount;
prefixCount[0] = 1;
int currSum = 0, count = 0;
for (int x : arr) {
    currSum += x;
    if (prefixCount.count(currSum - target))
        count += prefixCount[currSum - target];
    prefixCount[currSum]++;
}
```

---

## 📊 Master Complexity Cheat Sheet

### Data Structure Operations

| Data Structure | Insert | Delete | Search | Access by Index | Space |
|---------------|--------|--------|--------|-----------------|-------|
| **Array** (unsorted) | O(N)¹ | O(N) | O(N) | O(1) | O(N) |
| **Array** (sorted) | O(N)¹ | O(N) | O(log N) | O(1) | O(N) |
| **Linked List** (singly) | O(1) head / O(N) tail | O(N)² | O(N) | O(N) | O(N) |
| **Linked List** (doubly) | O(1) head/tail | O(1) given node | O(N) | O(N) | O(N) |
| **Hash Map** (`unordered_map`) | O(1) avg | O(1) avg | O(1) avg | N/A | O(N) |
| **BST** (balanced) | O(log N) | O(log N) | O(log N) | N/A | O(N) |
| **BST** (worst case) | O(N) | O(N) | O(N) | N/A | O(N) |
| **Min/Max Heap** | O(log N) | O(log N) | O(N)³ | N/A | O(N) |
| **Trie** | O(L)⁴ | O(L) | O(L) | N/A | O(ΣNLA)⁵ |
| **Graph** (adj. list) | O(1) edge | O(E) edge | O(V+E) BFS/DFS | N/A | O(V+E) |
| **Stack / Queue** | O(1) | O(1) | O(N) | N/A | O(N) |

**Notes:**
1. ¹ O(1) amortized if using `push_back` in dynamic array / vector
2. ² O(1) if you have pointer to previous node (doubly linked)
3. ³ Heap supports O(1) peek for min/max only, arbitrary search is O(N)
4. ⁴ L = length of key string
5. ⁵ Σ = alphabet size, N = number of words, LA = average length

### Algorithm Complexities Quick Reference

| Algorithm | Time | Space | Notes |
|-----------|------|-------|-------|
| Binary Search | O(log N) | O(1) | Sorted array required |
| Kadane's | O(N) | O(1) | Max subarray sum |
| Two Pointers | O(N) | O(1) | Sorted array or linked list |
| Sliding Window | O(N) | O(1) or O(K) | Subarray/substring problems |
| DFS / BFS | O(V + E) | O(V) | Graph/tree traversal |
| Dijkstra's | O((V+E) log V) | O(V) | Shortest path (no negative weights) |
| Topological Sort | O(V + E) | O(V) | DAG ordering |
| Floyd's Cycle | O(N) | O(1) | Linked list cycle detection |
| Union-Find | O(α(N)) ≈ O(1) | O(N) | Connected components, Kruskal's |

---

> **Final Tip**: When coding live at ION, always:
> 1. **Clarify** → ask 1-2 questions before coding
> 2. **State approach** → "I'll use Kadane's algorithm, which is O(N) time, O(1) space"
> 3. **Code** → write clean, compilable C++
> 4. **Test** → walk through with a small example
> 5. **Edge cases** → mention empty input, single element, all same values, negatives
