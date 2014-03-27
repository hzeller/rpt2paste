/* -*- c++ -*-
 * (c) h.zeller@acm.org. Free Software. GNU Public License v3.0 and above
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

#include "rpt-parser.h"
#include "rpt2paste.h"

static const float minimum_milliseconds = 50;
static const float area_to_milliseconds = 25;  // mm^2 to milliseconds.

// Smallest point from origin.
static float offset_x = 50;
static float offset_y = 50;

#define Z_DISPENSING "1.7"        // Position to dispense stuff. Just above board.
#define Z_HOVER_DISPENSER "2.5"   // Hovering above position
#define Z_HIGH_UP_DISPENSER "5"   // high up to separate paste.

class Printer {
public:
    virtual ~Printer() {}
    virtual void Init(float min_x, float min_y, float max_x, float max_y) = 0;
    virtual void Pad(float x, float y, float area) = 0;
    virtual void Finish() = 0;
};

class GCodePrinter : public Printer {
public:
    virtual void Init(float min_x, float min_y, float max_x, float max_y) {
        // G-code preamble. Set feed rate, homing etc.
        printf(
               //    "G28\n" assume machine is already homed before g-code is executed
               "G21\n" // set to mm
               "G0 F20000\n"
               "G1 F4000\n"
               "G0 Z4\n" // X0 Y0 may be outside the reachable area, and no need to go there
               );
    }

    virtual void Pad(float x, float y, float area) {
        printf(
               "G0 X%.3f Y%.3f Z" Z_HOVER_DISPENSER "\n"  // move to new position, above board
               "G1 Z" Z_DISPENSING "\n"                   // ready to dispense.
               "M106\n"               // switch on fan (=solenoid)
               "G4 P%.1f\n"           // Wait given milliseconds; dependend on area.
               "M107\n"               // switch off fan
               "G1 Z" Z_HIGH_UP_DISPENSER "\n", // high above to have paste is well separated
               x, y, minimum_milliseconds + area * area_to_milliseconds);
    }

    virtual void Finish() {
        printf(";done\n");
    }
};

class PostScriptPrinter : public Printer {
    virtual void Init(float min_x, float min_y, float max_x, float max_y) {
        min_x -= 3; min_y -=3; max_x +=3; max_y += 3;
        const float mm_to_point = 1 / 25.4 * 72.0;
        printf("%%!PS-Adobe-3.0\n%%%%BoundingBox: %.0f %.0f %.0f %.0f\n\n",
               min_x * mm_to_point, min_y * mm_to_point,
               max_x * mm_to_point, max_y * mm_to_point);
        printf("%% PastePad. Stack: <diameter>\n/pp { 0.2 setlinewidth 0 360 arc stroke } def\n\n"
               "%% Move. Stack: <x> <y>\n/m { 0.01 setlinewidth lineto currentpoint stroke } def\n\n");
        printf("72.0 25.4 div dup scale  %% Switch to mm\n");
        printf("%.1f %.1f moveto\n", offset_x - 10, offset_y - 10);
    }

    virtual void Pad(float x, float y, float area) {
        printf("%.3f %.3f m %.3f pp \n%.3f %.3f moveto ",
               x, y, sqrtf(area / M_PI), x, y);
    }

    virtual void Finish() {
        printf("showpage\n");
    }
};

class PadCollector : public ParseEventReceiver {
public:
    PadCollector(std::vector<const Pad*> *pads) : origin_x_(0), origin_y_(0), current_pad_(NULL),
                                                  collected_pads_(pads) {}

    virtual void StartComponent() { assert(current_pad_ == NULL); }
    virtual void EndComponent() { }

    virtual void StartPad() { current_pad_ = new Pad(); }
    virtual void EndPad() {
        if (current_pad_->drill != 0)
            delete current_pad_;  // through-hole. We're not interested in that.
        else
            collected_pads_->push_back(current_pad_);
        current_pad_ = NULL;
    }

    virtual void Position(float x, float y) {
        if (current_pad_ != NULL) {
            rotateXY(&x, &y);
            current_pad_->x = origin_x_ + x;
            current_pad_->y = origin_y_ + y;
        }
        else {
            origin_x_ = x;
            origin_y_ = y;
        }
    }
    virtual void Size(float w, float h) {
        if (current_pad_ == NULL) return;
        current_pad_->area = w * h;
    }

    virtual void Drill(float size) {
        assert(current_pad_ != NULL);
        current_pad_->drill = size;
    }

    virtual void Orientation(float angle) {
        if (current_pad_ == NULL) {
            // Angle is in degrees, make that radians.
            // mmh, and it looks like it turned in negative direction ? Probably part
            // of the mirroring.
            angle_ = -M_PI * angle / 180.0;
        }
    }

private:
    void rotateXY(float *x, float *y) {
        float xnew = *x * cos(angle_) - *y * sin(angle_);
        float ynew = *x * sin(angle_) + *y * cos(angle_);
        *x = xnew;
        *y = ynew;
    }

    // Current coordinate system.
    float origin_x_;
    float origin_y_;
    float angle_;

    // If we have seen a start-pad, this is not-NULL.
    Pad *current_pad_;
    std::vector<const Pad*> *collected_pads_;
};

static int usage(const char *prog) {
    fprintf(stderr, "Usage: %s <options> <rpt-file>\n"
            "Options:\n\t-p    : Output as PostScript\n",
            prog);
    return 1;
}
int main(int argc, char *argv[]) {
    bool do_postscript = false;

    int opt;
    while ((opt = getopt(argc, argv, "p")) != -1) {
        switch (opt) {
        case 'p':
            do_postscript = true;
            break;
        default: /* '?' */
            return usage(argv[0]);
        }
    }

    if (optind >= argc) {
        return usage(argv[0]);
    }

    const char *rpt_file = argv[optind];

    std::vector<const Pad*> pads;
    PadCollector collector(&pads);
    std::ifstream in(rpt_file);
    RptParse(&in, &collector);

    // The coordinates coming out of the file are mirrored, so we determine the maximum
    // to mirror at these axes.
    // (mmh, looks like it is only mirrored on y axis ?)
    float min_x = pads[0]->x, min_y = pads[0]->y;
    float max_x = pads[0]->x, max_y = pads[0]->y;
    for (size_t i = 0; i < pads.size(); ++i) {
        min_x = std::min(min_x, pads[i]->x);
        min_y = std::min(min_y, pads[i]->y);
        max_x = std::max(max_x, pads[i]->x);
        max_y = std::max(max_y, pads[i]->y);
    }

    Printer *printer;
    if (do_postscript)
        printer = new PostScriptPrinter();
    else
        printer = new GCodePrinter();

    OptimizePads(&pads);

    printer->Init(offset_x, offset_y,
                  (max_x - min_x) + offset_x, (max_y - min_y) + offset_y);

    for (size_t i = 0; i < pads.size(); ++i) {
        const Pad *pad = pads[i];
        // We move x-coordinates relative to the smallest X.
        // Y-coordinates are mirrored at the maximum Y (that is how the come out of the file)
        printer->Pad(pad->x + offset_x - min_x,
                     max_y - pad->y + offset_y,
                     pad->area);
    }

    printer->Finish();

    fprintf(stderr, "Dispensed %zd pads.\n", pads.size());
    for (size_t i = 0; i < pads.size(); ++i) {
        delete pads[i];
    }
    delete printer;
    return 0;
}
