#include <bits/stdc++.h>
#include "game.h"

using namespace std;

static const array<char,5> OPS = {'A','B','C','D','E'};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Read entire stdin into a string, so we can feed the simulator
    std::ostringstream oss;
    oss << cin.rdbuf();
    std::string input = oss.str();
    if (input.empty()) return 0;

    std::istringstream iss(input);
    auto game = std::make_unique<Game>(iss);
    // debug
    // cerr << "Parsed k=" << game.k << " n=" << game.n << " m=" << game.m << " s=" << game.s << "\n";

    vector<char> decisions;
    decisions.reserve(min<int>(game->m, 4 * game->n * game->n + 16));

    // Greedy per-bounce lookahead of depth 1
    int stallCount = 0;
    const int maxStallBeforeNudge = 8; // after some stalls, try to nudge vx toward small magnitude

    while (game->bricksRemaining() > 0 && (int)decisions.size() < game->m) {
        int bestHit = -1;
        int bestReward = INT_MIN;
        char bestOp = 'C';

        // Try all options and pick the one with most bricks hit (tie-break by reward)
        for (char op : OPS) {
            auto *snapshot = game->save();
            int reward = game->play(op);
            int hit = game->touch_cnt; // bricks hit during this move
            // Prefer more hits; tie-break by higher reward (captures 1-2-3 bonus)
            if (hit > bestHit || (hit == bestHit && reward > bestReward)) {
                bestHit = hit;
                bestReward = reward;
                bestOp = op;
            }
            game->load(snapshot);
            game->erase(snapshot);
        }

        // If no brick would be hit this cycle, try a cheap 2-step lookahead;
        // if still none, steer vx toward small non-zero magnitude
        if (bestHit <= 0) {
            ++stallCount;
            int bestFuture = -1;
            char opFromLookahead = 'C';
            for (char op1 : OPS) {
                auto *s1 = game->save();
                (void)game->play(op1);
                int localBest = 0;
                for (char op2 : OPS) {
                    auto *s2 = game->save();
                    (void)game->play(op2);
                    localBest = max(localBest, game->touch_cnt);
                    game->load(s2);
                    game->erase(s2);
                }
                if (localBest > bestFuture) {
                    bestFuture = localBest;
                    opFromLookahead = op1;
                }
                game->load(s1);
                game->erase(s1);
            }
            if (bestFuture > 0) {
                bestOp = opFromLookahead;
            } else {
                int curVx = game->situation_now.ball.vx;
                int targetAbs = (stallCount >= maxStallBeforeNudge) ? 1 : 2; // try 2 then 1
                int bestAbsAfter = INT_MAX;
                char steerOp = 'C';
                for (char op : OPS) {
                    int dv = game->check_op(op);
                    int nv = curVx + dv;
                    int cand = std::abs(nv);
                    // prefer non-zero and as close to targetAbs as possible; tie-break smaller abs
                    int primary = std::abs(cand - targetAbs);
                    int secondary = cand;
                    if ((cand > 0 && (primary < std::abs(bestAbsAfter - targetAbs) ||
                                       (primary == std::abs(bestAbsAfter - targetAbs) && secondary < bestAbsAfter))) ||
                        (bestAbsAfter == INT_MAX)) {
                        bestAbsAfter = cand;
                        steerOp = op;
                    }
                }
                bestOp = steerOp;
            }
        } else {
            stallCount = 0;
        }

        // Apply chosen operation to advance state and record output
        game->play(bestOp);
        decisions.push_back(bestOp);
    }

    // Output decisions, one per line
    for (char c : decisions) {
        cout << c << '\n';
    }

    return 0;
}
