// ============================================================
// DRILL 7 — SNAKE & LADDER (LLD, fresher-level, runnable)
// Escalations to SAY (he will ask):
//   1. "N players?" -> players is already a vector — add more, zero changes.
//   2. "Exact 100 to win?" -> already handled: p + r > 100 means no move.
//   3. "Multiple dice / 2 sixes bonus roll?" -> loop the roll; easy extension.
//   4. "Undo last move?" -> keep a stack of (player, oldPos) — pop on undo.
//   5. "Save/resume game?" -> store board maps + each player's pos (game state).
//   6. "Two players same snake head?" -> impossible: state is just positions;
//      snakes/ladders are pure rules on the board, not per player.
// COMPILE & RUN:  g++ -std=c++17 07_snake_ladder.cpp -o g && ./g
// PREDICTED OUTPUT: varies (random dice) — ends with "X WINS!"
// ============================================================

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>
using namespace std;

class Player {
public:
    string name;
    int pos;                       // 0 = not started, 1..100 on board
    Player(string n) { name = n; pos = 0; }
};

class Dice {
public:
    int roll() { return rand() % 6 + 1; }
};

class Board {
public:
    map<int, int> snakes, ladders;             // head -> tail, base -> top
    Board() {
        snakes[99] = 54;  snakes[70] = 55;  snakes[52] = 40;
        ladders[4] = 25;  ladders[13] = 46; ladders[33] = 49;
    }
    int finalPos(int pos) {                    // apply snake/ladder AFTER landing
        if (snakes.count(pos)) {
            cout << "   SNAKE! " << pos << " slides to " << snakes[pos] << endl;
            return snakes[pos];
        }
        if (ladders.count(pos)) {
            cout << "   LADDER! " << pos << " climbs to " << ladders[pos] << endl;
            return ladders[pos];
        }
        return pos;
    }
};

class Game {
    vector<Player> players;
    Board board;
    Dice dice;
public:
    void addPlayer(string name) { players.push_back(Player(name)); }

    void play() {
        while (true) {
            for (int i = 0; i < (int)players.size(); i++) {      // turn by turn
                int r = dice.roll();
                int p = players[i].pos;
                if (p + r > 100) {                               // exact 100 needed
                    cout << players[i].name << " rolled " << r
                         << " (needs exact) - stays at " << p << endl;
                    continue;
                }
                players[i].pos = board.finalPos(p + r);
                cout << players[i].name << " rolled " << r
                     << " -> at " << players[i].pos << endl;
                if (players[i].pos == 100) {
                    cout << players[i].name << " WINS!\n";
                    return;
                }
            }
        }
    }
};

int main() {
    srand(time(0));                        // seed randomness once
    Game g;
    g.addPlayer("Ritik");
    g.addPlayer("Priya");
    g.play();
    return 0;
}
