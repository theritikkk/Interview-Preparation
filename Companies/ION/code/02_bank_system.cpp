// ============================================================
// DRILL 2 — "WRITE A BANKING SYSTEM AND RUN IT" [CAND — some rounds ask this]
// Shows: inheritance, virtual overriding (type / interest / withdraw limit),
//        encapsulation (private balance + history), validation, runtime dispatch.
//
// COMPILE & RUN:  g++ -std=c++17 02_bank_system.cpp -o b && ./b
//
// PREDICTED OUTPUT (say it aloud BEFORE running):
//   Savings account of Ritik : Rs.50500 | interest 4%/yr
//     history of Ritik:
//       opened with Rs.50000
//       deposited 1000
//       withdrew 500
//   withdraw of 900000 refused (balance/limit)
//   Current account of ShopA : Rs.120500 | interest 0%/yr
//     history of ShopA:
//       opened with Rs.120000
//       deposited 1000
//       withdrew 500
// ============================================================

#include <iostream>
#include <string>
#include <vector>
using namespace std;

class Account {
private:
    string owner;
    int balance;                 // whole rupees (floats cause rounding bugs — say this)
    vector<string> history;      // private audit trail
public:
    Account(string owner, int opening) : owner(owner), balance(opening) {
        history.push_back("opened with Rs." + to_string(opening));
    }
    virtual ~Account() { }

    virtual string type() { return "Account"; }            // virtual hooks
    virtual double interestPerYear() { return 0; }
    virtual int withdrawLimit() { return 100000; }

    void deposit(int amt) {
        if (amt <= 0) { cout << "  invalid deposit amount\n"; return; }
        balance += amt;
        history.push_back("deposited " + to_string(amt));
    }
    void withdraw(int amt) {
        if (amt <= 0 || amt > balance || amt > withdrawLimit()) {
            cout << "  withdraw of " << amt << " refused (balance/limit)\n";
            return;
        }
        balance -= amt;
        history.push_back("withdrew " + to_string(amt));
    }
    void showHistory() {
        cout << "  history of " << owner << ":\n";
        for (int i = 0; i < (int)history.size(); i++)
            cout << "    " << history[i] << "\n";
    }
    void statement() {
        cout << type() << " account of " << owner << " : Rs." << balance
             << " | interest " << interestPerYear() << "%/yr\n";
    }
    int getBalance() { return balance; }
};

class SavingsAccount : public Account {
public:
    SavingsAccount(string owner, int opening) : Account(owner, opening) {}
    string type() override { return "Savings"; }
    double interestPerYear() override { return 4; }
};

class CurrentAccount : public Account {
public:
    CurrentAccount(string owner, int opening) : Account(owner, opening) {}
    string type() override { return "Current"; }
    int withdrawLimit() override { return 500000; }        // businesses move more money
};

int main() {
    Account* a1 = new SavingsAccount("Ritik", 50000);
    Account* a2 = new CurrentAccount("ShopA", 120000);

    a1->deposit(1000);
    a1->withdraw(500);
    a1->statement();
    a1->showHistory();

    a2->deposit(1000);
    a2->withdraw(500);
    a2->withdraw(900000);        // REFUSED: over Current's limit — same withdraw code,
                                 // different limit per type = virtual dispatch, live
    a2->statement();
    a2->showHistory();

    delete a1;
    delete a2;
    return 0;
}
