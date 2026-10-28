// ============================================================
// DRILL 4 — TRAFFIC LIGHT (LLD, fresher-level skeleton)
// The 5 things to SAY while showing this:
//   1. phase = a set of roads green TOGETHER (conflict-free by construction)
//   2. GREEN -> AMBER(3s) -> ALL-RED(2s) -> next phase (the all-red kills
//      the "both yellow" race)
//   3. IDs are names, not array indexes — a new road never breaks old ones
//   4. pedestrians walk only PARALLEL to the green they cross against
//   5. emergency = never jump straight to green; finish amber + all-red first
//   6. any fault -> flashing red / all-red (fail-safe)
// COMPILE & RUN:  g++ -std=c++17 04_traffic_light.cpp -o t && ./t
// ============================================================

#include <iostream>
#include <string>
using namespace std;

enum State { RED, GREEN, AMBER };

class Signal {
public:
    string road;      // road NAME = stable ID (never an index)
    State state;
    Signal(string r) { road = r; state = RED; }
    void show() {
        cout << "  " << road << " = "
             << (state == GREEN ? "GREEN" : state == AMBER ? "AMBER" : "RED") << endl;
    }
};

class Controller {
    Signal* phase[2];   // phase 0 = North+South pair, phase 1 = East+West pair
    int current;        // which phase is green right now (0 or 1)
public:
    Controller() {
        phase[0] = new Signal("North+South");
        phase[1] = new Signal("East+West");
        current = 0;
    }
    ~Controller() { delete phase[0]; delete phase[1]; }

    void runPhase() {
        cout << "-- phase " << current << " --\n";
        phase[current]->state = GREEN;
        phase[current]->show();

        phase[current]->state = AMBER;                 // 3 s warning
        phase[current]->show();

        phase[current]->state = RED;
        cout << "  (ALL-RED clearance: both phases red for 2 s)\n";

        current = 1 - current;                         // next phase
    }

    void emergency() {   // never jump straight to green:
        cout << "EMERGENCY: finish current phase safely first ->\n";
        runPhase();        // amber + all-red happen inside = the safety
    }
};

int main() {
    Controller c;
    c.runPhase();        // North+South green -> amber -> all-red
    c.runPhase();        // East+West  green -> amber -> all-red
    c.emergency();       // safe transition, never instant
    return 0;
}
