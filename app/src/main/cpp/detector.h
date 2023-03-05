#ifndef DETECTOR_H
#define DETECTOR_H

#include <opencv2/core.hpp>

using namespace cv;
using namespace std;

typedef struct Quadrilateral
{
    Point p1, p2, p3, p4;
    float area;
} Quadrilateral;

typedef Vec4i Line;

typedef struct LineIntersecPack
{
    Line a;
    Line b;
    Point intersection;
    int score;
    int angle;
} LineIntersecPack;

//bool timerOn(bool start);

int getTime();

void setMask(bool mask);

bool get_outline(const Mat &gray, Mat &threshold, Mat &edges, Quadrilateral& outline, bool timerOn, bool preview = true, Mat debug = Mat());

vector<Point> getIntersections();

void warp_document(const Mat& src, Quadrilateral region, Mat& dst);

#endif
