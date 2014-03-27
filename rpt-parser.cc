/* -*- c++ -*-
 * (c) h.zeller@acm.org. Free Software. GNU Public License v3.0 and above
 */

#include <string>
#include <iostream>

#include "rpt-parser.h"

// Very crude parser. No error handling. Quick hack.
bool RptParse(std::istream *input, ParseEventReceiver *event) {
    while (!input->eof()) {
        std::string token;
        (*input) >> token;
        if (token == "$MODULE")
            event->StartComponent();
        else if (token == "$EndMODULE")
            event->EndComponent();
        else if (token == "$PAD")
            event->StartPad();
        else if (token == "$EndPAD")
            event->EndPad();
        else if (token == "position") {
            float x, y;
            (*input) >> x >> y;
            event->Position(x, y);
        }
        else if (token == "size") {
            float w, h;
            (*input) >> w >> h;
            event->Size(w, h);
        }
        else if (token == "drill") {
            float dia;
            (*input) >> dia;
            event->Drill(dia);
        }
        else if (token == "orientation") {
            float angle;
            (*input) >> angle;
            event->Orientation(angle);
        }
    }
    return true;
}
