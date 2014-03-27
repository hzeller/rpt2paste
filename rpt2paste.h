/* -*- c++ -*-
 * (c) h.zeller@acm.org. Free Software. GNU Public License v3.0 and above
 */

#include <vector>

struct Pad {
    Pad() : x(0), y(0), drill(0), area(0) {}
    float x, y;
    float drill;
    float area;
};

// Find acceptable route for pad visiting. Ideally solves TSP, but heuristics are good
// as well.
void OptimizePads(std::vector<const Pad*> *pads);
