#include "detector.h"

#include <opencv2/imgproc.hpp>

#include <math.h>

#include <iostream>

#include "logging.h"

#include <time.h>


using namespace cv;
using namespace std;

#define PI 3.14159265
#define SIN(angle) sin(angle * PI / 180)
#define COS(angle) cos(angle * PI / 180)

#define SCALE_RATIO 0.5
#define MIN_MERGE_DISTANCE 10
#define MIN_MERGE_ANGLE 1
#define MAX_ANGLE_THRESH 20
#define FILTER_ANGLE_THRESH 10

#define radiansToDegrees(angleRadians) ((angleRadians) * 180.0 / CV_PI)

#define LINE_SHRINK 0.05

#define BEGIN_POINT(L) Point((L)[0], (L)[1])
#define END_POINT(L) Point((L)[2], (L)[3])

#define MAKE_LINE(P1, P2) Line((P1).x, (P1).y, (P2).x, (P2).y)


struct lineXSort {
    inline bool operator() (const Line& a, const Line& b) {
        return (a[0] < b[0]);
    }
};

struct lineYSort {
    inline bool operator() (const Line& a, const Line& b) {
        return (a[1] < b[1]);
    }
};

struct mergedXSort {
    inline bool operator() (const Line& a, const Line& b) {
        return ((a[1] + a[3]) > (b[1] + b[3]));
    }
};

struct mergedYSort {
    inline bool operator() (const Line& a, const Line& b) {
        return ((a[0] + a[2]) > (b[0] + b[2]));
    }
};

struct pointYSort {
    inline bool operator() (const Point& a, const Point& b) {
        return (a.y < b.y);
    }
};

struct pointXSort {
    inline bool operator() (const Point& a, const Point& b) {
        return (a.x < b.x);
    }
};


struct quadPtsSort {
    inline bool operator() (const Point& left, const Point& right) {
        return (left.x < right.x) || ((left.x == right.x) && (left.y < right.y));
    }
};

vector<Point> combinations;
vector<Line> combinationsLine;
int counter = 0;
Mat temp1;
static Quadrilateral savedQuad;
static int preCounter = 100;

vector<LineIntersecPack> findIntersections(vector<Line> all, Mat dst, Mat resized);

static double distanceBtwPoints(const cv::Point2f &a, const cv::Point2f &b) {
    double xDiff = a.x - b.x;
    double yDiff = a.y - b.y;

    return std::sqrt((xDiff * xDiff) + (yDiff * yDiff));
}

double Slope(int x0, int y0, int x1, int y1) {
    int x = x0 - x1;
    int y = y0 - y1;
    if(x == 0) { return 10 * y; }
    return (double) y / x;
}


int getMagnitude(int x1, int y1,int x2, int y2) {
    return sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}

/*
int distancePointLine(int px, int py, int x1, int y1, int x2, int y2) {
    int distancePointLine,ix,iy;
    int lineMagnitude = getMagnitude(x1, y1, x2, y2);

    if (lineMagnitude < 0.00000001)
        return 9999;

    int u1 = (((px - x1) * (x2 - x1)) + ((py - y1) * (y2 - y1)));
    int u = u1 /(int)(pow(lineMagnitude, 2));

    if (u < 0.00001 || u > 1) { // closest point does not fall withing the line segment, take shorter distance
        ix = getMagnitude(px, py, x1, y1);
        iy = getMagnitude(px, py, x2, y2);

        distancePointLine = ix > iy ? iy : ix;

    }
    else {
        ix = x1 + u * (x2 - x1);
        iy = y1 + u * (y2 - y1);
        distancePointLine = getMagnitude(px, py, ix, iy);
    }

    return distancePointLine;
}
 */

int distancePointLine2(int px, int py, int x1, int y1, int x2, int y2) {
    double k1 = Slope(x1, y1, x2, y2);
    if (k1 == 0)
        k1 = 0.0001;
    double k2 = -1 / k1; //pravokotnica

    double n1 = y1 - x1 * k1;
    double n2 = py - px * k2;

    double t_x = (n2 - n1) / (k1 - k2); //presečišče x
    double t_y = t_x * k1 + n1;         //presečišče y
    int tx = std::round(t_x);
    int ty = std::round(t_y);

    if (tx <= x1 && tx >= x2 || tx >= x1 && tx <= x2) { //
        if (tx == x1 && tx == x2)
            if(ty <= y1 && ty >= y2 || ty >= y1 && ty <= y2)
                return getMagnitude(px, py, tx, ty);
            else
                return std::min(getMagnitude(px, py, x1, y1), getMagnitude(px, py, x2, y2));
        return getMagnitude(px, py, tx, ty);
    }
    return std::min(getMagnitude(px, py, x1, y1), getMagnitude(px, py, x2, y2));

}

int getDistance(Line line1, Line line2) {
    int dist1, dist2, dist3, dist4;
    int dist11, dist21, dist31, dist41;

    //dist1 = distancePointLine(line1[0], line1[1], line2[0], line2[1], line2[2], line2[3]);
    //dist2 = distancePointLine(line1[2], line1[3], line2[0], line2[1], line2[2], line2[3]);
    //dist3 = distancePointLine(line2[0], line2[1], line1[0], line1[1], line1[2], line1[3]);
    //dist4 = distancePointLine(line2[2], line2[3], line1[0], line1[1], line1[2], line1[3]);

    dist11 = distancePointLine2(line1[0], line1[1], line2[0], line2[1], line2[2], line2[3]);
    dist21 = distancePointLine2(line1[2], line1[3], line2[0], line2[1], line2[2], line2[3]);
    dist31 = distancePointLine2(line2[0], line2[1], line1[0], line1[1], line1[2], line1[3]);
    dist41 = distancePointLine2(line2[2], line2[3], line1[0], line1[1], line1[2], line1[3]);

    //vector<int> distances = { dist1,dist2,dist3,dist4 };
    vector<int> distances = { dist11,dist21,dist31,dist41 };
    std::vector<int>::iterator min = std::min_element(distances.begin(), distances.end());
    return *min;
}

Line mergeLineSegments(vector<Line> lines) {
    if (lines.size() == 1)
        return lines[0];

    Line lineI = lines[0];

    double orientationI = atan2(lineI[1] - lineI[3], lineI[0] - lineI[2]);

    vector<Point> points;

    for (size_t i = 0; i < lines.size(); i++) {
        points.push_back(Point(lines[i][0], lines[i][1]));
        points.push_back(Point(lines[i][2], lines[i][3]));
    }
    int degrees = abs(radiansToDegrees(orientationI));
    if (degrees > 45 && degrees < 135) {
        std::sort(points.begin(), points.end(), pointYSort());

    }
    else {
        std::sort(points.begin(), points.end(), pointXSort());

    }
    return Line(points[0].x, points[0].y, points[points.size() - 1].x, points[points.size() - 1].y);
}

vector<Line> mergeLines(vector<Line> lines, int minMergeDistance, int minMergeAngle) {
    vector<vector<Line>> superLines;
    vector<Line> superLinesFinal;

    // Merge lines with similar angles to groups
    for (size_t i = 0; i < lines.size(); i++) {
        bool createNewGroup = true;
        bool groupUpdated = false;

        // iterate through groups
        for (size_t j = 0; j < superLines.size(); j++) {
            // iterate through a group
            for (size_t k = 0; k < superLines[j].size(); k++) {
                // first check the distance
                Line line2 = superLines[j][k];
                if (getDistance(line2, lines[i]) < minMergeDistance) {
                    // check the angle
                    double orientationI = atan2(lines[i][1] - lines[i][3], lines[i][0] - lines[i][2]);
                    double orientation2 = atan2(line2[1] - line2[3], line2[0] - line2[2]);

                    if (abs(abs(radiansToDegrees(orientationI)) - abs(radiansToDegrees(orientation2))) < minMergeAngle) {
                    //if (abs(abs(radiansToDegrees(orientationI)) - abs(radiansToDegrees(orientationI))) < minMergeAngle)
                        superLines[j].push_back(lines[i]);
                        createNewGroup = false;
                        groupUpdated = true;
                        break;
                    }

                }
            } // through a group
            if (groupUpdated)
                break;
        } // groups

        if (createNewGroup) {
            vector<Line> newGroup;
            newGroup.push_back(lines[i]);

            for (size_t z = 0; z < lines.size(); z++) {
                Line line2 = lines[z];
                if (getDistance(lines[z], lines[i]) < minMergeDistance) {
                    double orientationI = atan2(lines[i][1] - lines[i][3], lines[i][0] - lines[i][2]);
                    double orientationZ = atan2(lines[z][1] - lines[z][3], lines[z][0] - lines[z][2]);

                    if (abs(abs(radiansToDegrees(orientationI)) - abs(radiansToDegrees(orientationZ))) < minMergeAngle) {
                    //if (abs(abs(radiansToDegrees(orientationI)) - abs(radiansToDegrees(orientationZ))) < minMergeAngle)
                        newGroup.push_back(line2);

                    }
                }
            }
            superLines.push_back(newGroup);
        }
    } // lines

    for (size_t j = 0; j < superLines.size(); j++) {
        superLinesFinal.push_back(mergeLineSegments(superLines[j]));
    }

    return superLinesFinal;
}

vector<Line> processLines(vector<Vec4i> raw_lines, int minMergeDist, int minMergeAngle)
{
    vector<Line> lines,linesX,linesY;
    // store the lines to make them more readable
    for (size_t i = 0; i < raw_lines.size(); i++) {
        Vec4i l = raw_lines[i];
        lines.push_back(Line(l[0], l[1], l[2], l[3]));
    }

    for (size_t i = 0; i < lines.size(); i++) {
        // Retrieve the orientation and sort them into x and y
        double orientation = atan2(lines[i][1] - lines[i][3], lines[i][0] - lines[i][2]);
        int degrees = abs(radiansToDegrees(orientation));
        if (degrees > 45 && degrees < 135)
            linesY.push_back(lines[i]);
        else
            linesX.push_back(lines[i]);
    }

    std::sort(linesX.begin(), linesX.end(), lineXSort());
    std::sort(linesY.begin(), linesY.end(), lineYSort());

    vector<Line> mergedX = mergeLines(linesX, minMergeDist, minMergeAngle);
    vector<Line> mergedY = mergeLines(linesY, minMergeDist, minMergeAngle);

    std::sort(mergedX.begin(), mergedX.end(), mergedXSort());
    std::sort(mergedY.begin(), mergedY.end(), mergedYSort());

    // Filter out inner lines
    vector<Line> mergedXn, mergedYn;
    int linesPerEdge = 3;
    if(mergedX.size() <= linesPerEdge*2)
        mergedXn = mergedX;
    else {
        for (int i = 0; i < linesPerEdge; ++i) {
            mergedXn.push_back(mergedX[i]);
            mergedXn.push_back(mergedX[mergedX.size() - 1 - i]);
        }
    }
    if(mergedY.size() <= linesPerEdge*2)
        mergedYn = mergedY;
    else {
        for (int i = 0; i < linesPerEdge; ++i) {
            mergedYn.push_back(mergedY[i]);
            mergedYn.push_back(mergedY[mergedY.size() - 1 - i]);
        }
    }

    vector<Line> all;
    all.reserve(all.size() + mergedXn.size() + mergedYn.size());
    all.insert(all.end(), mergedXn.begin(), mergedXn.end());
    all.insert(all.end(), mergedYn.begin(), mergedYn.end());

    return all;
}

pair<Point, Point> getFullLine(cv::Point a, cv::Point b, Size bounds) {

    double slope = Slope(a.x, a.y, b.x, b.y);

    Point p(0, 0), q(bounds.width, bounds.height);

    p.y = -(a.x - p.x) * slope + a.y;
    q.y = -(b.x - q.x) * slope + b.y;
    cv::clipLine(bounds, p, q);

    return make_pair(p, q);
}

bool getIntersection(Line a, Line b, Point2f &r, Point2f &r2, Size bounds) {

    pair<Point, Point> aPts = getFullLine(BEGIN_POINT(a), END_POINT(a), bounds);
    pair<Point, Point> bPts = getFullLine(BEGIN_POINT(b), END_POINT(b), bounds);

    Point2f o1 = aPts.first;
    Point2f p1 = aPts.second;

    Point2f o2 = bPts.first;
    Point2f p2 = bPts.second;

    Point2f x = o2 - o1;
    Point2f d1 = p1 - o1;
    Point2f d2 = p2 - o2;

    if(d1.x == 0)
        d1.x = 0.0001;
    if(d2.x == 0)
        d2.x = 0.0001;
    float p1k = d1.y / d1.x;
    float p2k = d2.y / d2.x;

    float p1n = o1.y - o1.x * p1k;
    float p2n = o2.y - o2.x * p2k;

    if(p1k == p2k)
        return false;
    float tx = (p2n - p1n) / (p1k - p2k);
    r2 = {tx, tx * p1k + p1n};

    float cross = d1.x*d2.y - d1.y*d2.x;
    if (abs(cross) < /*EPS*/1e-8)
        return false;

    double t1 = (x.x * d2.y - x.y * d2.x) / cross;
    r = o1 + d1 * t1;
    if (r.x < 0 || r.y < 0 || r.y > bounds.height || r.x > bounds.width)
        return false;
    return true;
}

float pointDistSquared(Point2f cross, Point2f candidate) {
    return (cross.x - candidate.x) * (cross.x - candidate.x) + (cross.y - candidate.y) * (cross.y - candidate.y);
}

vector<Point> findIntersections(vector<Line> all, Size bounds) {
    for (size_t i = 0; i < all.size(); i++)  {
        pair<Point, Point> pts = getFullLine(BEGIN_POINT(all[i]), END_POINT(all[i]), bounds);
    }

    vector<Point> cornerCandidates;

    for (size_t i = 0; i < all.size(); i++)
        for (size_t j = i + 1; j < all.size(); j++) {
            Point2f cross;
            Point2f cross2;
            if (getIntersection(all[i], all[j], cross, cross2, bounds)) {
                size_t k;
                for (k = 0; k < cornerCandidates.size(); k++) {
                    if(pointDistSquared(cross, cornerCandidates[k]) <= 50) {
                        counter++;
                        break;
                    }
                }
                if(k < cornerCandidates.size())
                    break;

                int degreesA = radiansToDegrees(CV_PI - abs(atan(Slope(all[i][0],
                                                                       all[i][1],
                                                                       all[i][2],
                                                                       all[i][3])) -
                                                            atan(Slope(all[j][0],
                                                                       all[j][1],
                                                                       all[j][2],
                                                                       all[j][3]))));
                if (abs(degreesA - 90) < MAX_ANGLE_THRESH) { //  find the ones that have at least +- 10 to right angle at the intersection
                    //LineIntersecPack e = { all[i],all[j],cross,0 };
                    Point e = cross;
                    cornerCandidates.push_back(e);
                }
            }
        }

    return cornerCandidates;
}


void pointCombinations(int offset, int k,vector<Point> candidates,vector<vector<Point>> *combinationRes) {
    if (k == 0) {
        combinationRes->push_back(combinations);
        return;
    }
    for (int i = offset; i <= candidates.size() - k; ++i) {
        combinations.push_back(candidates[i]);
        pointCombinations(i + 1, k - 1, candidates,combinationRes);
        combinations.pop_back();
    }
}

void lineCombinations(int offset, int k, vector<Line> candidates, vector<vector<Line>> *combinationRes) {
    if (k == 0) {
        //pretty_print(combinations);
        combinationRes->push_back(combinationsLine);
        return;
    }
    for (int i = offset; i <= candidates.size() - k; ++i) {
        combinationsLine.push_back(candidates[i]);
        lineCombinations(i + 1, k - 1, candidates, combinationRes);
        combinationsLine.pop_back();
    }
}

// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are colinear
// 1 --> Clockwise
// 2 --> Counterclockwise
int orientation(Point p, Point q, Point r) {
    // See https://www.geeksforgeeks.org/orientation-3-ordered-points/
    // for details of below formula.
    int val = (q.y - p.y) * (r.x - q.x) -
              (q.x - p.x) * (r.y - q.y);

    if (val == 0) return 0;  // colinear

    return (val > 0) ? 1 : 2; // clock or counterclock wise
}

// The main function that returns true if line segment 'p1q1'
// and 'p2q2' intersect.
bool doIntersect(Point p1, Point q1, Point p2, Point q2) {
    // Find the four orientations needed for general and
    // special cases
    int o1 = orientation(p1, q1, p2);
    int o2 = orientation(p1, q1, q2);
    int o3 = orientation(p2, q2, p1);
    int o4 = orientation(p2, q2, q1);

    if (o1 == 0 || o2 == 0 || o3 == 0 || o4 == 0)
        return false;
    // General case
    if (o1 != o2 && o3 != o4)
        return true;
    return false; // Doesn't fall in any of the above cases
}



Quadrilateral constructQuad(Point p1, Point p2, Point p3, Point p4) {
    Point botleft, topleft, topright, bottomright;
    vector<Point> pts{p1,p2,p3,p4};
    Quadrilateral quad;
    std::sort(pts.begin(), pts.end(), quadPtsSort());
    // pts sorted by x min->max
    if (pts[0].y > pts[1].y) {
        botleft = pts[0];
        topleft = pts[1];
    }
    else {
        botleft = pts[1];
        topleft = pts[0];
    }
    if (pts[2].y > pts[3].y) {
        topright = pts[3];
        bottomright = pts[2];
    }
    else {
        topright = pts[2];
        bottomright = pts[3];
    }

    int degrees1 = radiansToDegrees(CV_PI -
                                    abs(atan(Slope(botleft.x, botleft.y, topleft.x, topleft.y)) -
                                        atan(Slope(topleft.x, topleft.y, topright.x, topright.y))));
    int degrees2 = radiansToDegrees(CV_PI -
                                    abs(atan(Slope(topleft.x, topleft.y, topright.x, topright.y)) -
                                        atan(Slope(topright.x, topright.y, bottomright.x, bottomright.y))));
    int degrees3 = radiansToDegrees(CV_PI -
                                    abs(atan(Slope(topright.x, topright.y, bottomright.x, bottomright.y)) -
                                        atan(Slope(bottomright.x, bottomright.y, botleft.x, botleft.y))));
    int degrees4 = radiansToDegrees(CV_PI -
                                    abs(atan(Slope(bottomright.x, bottomright.y, botleft.x, botleft.y)) -
                                        atan(Slope(botleft.x, botleft.y, topleft.x, topleft.y))));
    int thresh = FILTER_ANGLE_THRESH;
    int width_thresh = 30;
    if (abs(degrees1 - 90) < thresh && abs(degrees2 - 90) < thresh && abs(degrees3 - 90) < thresh && abs(degrees4 - 90) < thresh) {
        vector<Point> ctr{ botleft, topleft, topright, bottomright };
        float area = contourArea(ctr);
        quad = {botleft,topleft,topright,bottomright, area};
    }

    return quad;

}

bool scoreBestQuadrilateral(vector<Quadrilateral> quads, Quadrilateral &res) {
    int max = 0;
    int maxIdx = -1;
    for (size_t i = 0; i < quads.size(); i++) {
        if (quads[i].area > max) {
            max = quads[i].area;
            maxIdx = i;
        }
    }
    if (maxIdx < 0) return false;
    Quadrilateral finalQuad = quads[maxIdx];
    res = finalQuad;

    return true;
}

bool filterOptions(vector<Point> points, Quadrilateral &retQuad) {
    //filter in findIntersections

    combinations.clear();
    combinationsLine.clear();

    // generate all combinations and store them into combinations
    if(points.size() <= 1) return false;
    vector<vector<Point>> pointCombination;
    pointCombinations(0, 2, points, &pointCombination);
    vector<Line> allLines;

    for (size_t i = 0; i < pointCombination.size(); i++) {
        allLines.push_back(MAKE_LINE(pointCombination[i][0], pointCombination[i][1]));
    }

    vector<vector<Line>> lineComb;
    vector<Quadrilateral> quads;
    if(allLines.size() <= 1) return false;
    lineCombinations(0, 2, allLines, &lineComb); // TAKES MOST TIME

    int count = 0;
    for (size_t i = 0; i < lineComb.size(); i++) {
        vector<Line> line = lineComb[i];
        Point vecA = BEGIN_POINT(lineComb[i][0]) - END_POINT(lineComb[i][0]);
        Point vecB = BEGIN_POINT(lineComb[i][1]) - END_POINT(lineComb[i][1]);
        double tan1 = atan(Slope(lineComb[i][0][0], lineComb[i][0][1], lineComb[i][0][2], lineComb[i][0][3]));
        double tan2 = atan(Slope(lineComb[i][1][0], lineComb[i][1][1], lineComb[i][1][2], lineComb[i][1][3]));
        int degrees2 = radiansToDegrees(abs(tan1 - tan2));
        bool minAngleCondition2 = abs(degrees2) > MAX_ANGLE_THRESH;

        // Find all quadrilaterals that suffice the conditions
        bool inter = doIntersect(BEGIN_POINT(lineComb[i][0]) - vecA * LINE_SHRINK,
                                 END_POINT(lineComb[i][0]) + vecA * LINE_SHRINK,
                                 BEGIN_POINT(lineComb[i][1]) - vecB * LINE_SHRINK,
                                 END_POINT(lineComb[i][1]) + vecB * LINE_SHRINK);
        if (inter) {
            if(minAngleCondition2) {
                Quadrilateral quad = constructQuad(BEGIN_POINT(lineComb[i][0]),
                                                   END_POINT(lineComb[i][0]),
                                                   BEGIN_POINT(lineComb[i][1]),
                                                   END_POINT(lineComb[i][1]));
                if(quad.area)
                    quads.push_back(quad);
            }
            else
                count = 0;
        }
    }

    if(quads.size() == 0) return false;

    Quadrilateral quad = { Point(0,0),Point(0,0),Point(0,0),Point(0,0),0 };

    if (scoreBestQuadrilateral(quads, quad)) {
        retQuad = quad;
        return true;
    }
    return false;
}

int similarQuad(Quadrilateral& oldQuad, Quadrilateral& newQuad) {
    if(oldQuad.area == 0)
        return 0;
    int d1 = distanceBtwPoints(oldQuad.p1, newQuad.p1);
    int d4 = distanceBtwPoints(oldQuad.p4, newQuad.p4);
    if(d1 < 20 && d4 < 20)
        return d1 + d4 + 5;
    return 0;
}


Quadrilateral sortPoints(vector<Point> pts) {
    Point botleft, topleft, topright, bottomright;
    std::sort(pts.begin(), pts.end(), quadPtsSort());
    if (pts[0].y > pts[1].y) {
        botleft = pts[0];
        topleft = pts[1];
    }
    else {
        botleft = pts[1];
        topleft = pts[0];
    }
    if (pts[2].y > pts[3].y) {
        topright = pts[3];
        bottomright = pts[2];
    }
    else {
        topright = pts[2];
        bottomright = pts[3];
    }
    return {botleft,topleft,topright,bottomright, 0};
}


vector<Point> intersections;
bool startTimer = false;
bool masked = false;
double comb = 0;
int timeCount = 0;
int incr = 0;
double avg = 0;
/*
bool timerOn(bool start) {
    LOGD("TIMER SWITCHED");
    startTimer = start;
    if(!start && timeCount > 0)
        LOGD("Avg: %f\n", comb / timeCount);

    //comb = 0;
    //timeCount = 0;
}
 */

int getTime() {
    return (int) (avg * 1000);
}

void setMask(bool mask) {
    masked = mask;
}


bool get_outline(const Mat &gray, Mat &tmp1, Mat &tmp2, Quadrilateral& outline, bool timerOn, bool preview, Mat debug) {
    counter = 0;
    GaussianBlur(gray, tmp1, Size(5, 5), 0);

    struct timespec start, finish;
    double elapsed;
    double total = 0;
    clock_gettime(CLOCK_BOOTTIME, &start);

    int kernel[9] = {1, 1, 0, 1, 1, 1, 0, 1, 1};
    int kernel2[4] = {1, 1, 1, 1};

    //cv::threshold(tmp1, tmp2, 0, 255, THRESH_OTSU);

    Canny(tmp1, tmp2, 5, 35, 3, true);
    dilate(tmp2, tmp2, Mat(3, 3, CV_8U, kernel));//2, 2, CV_8U, kernel2

    Rect rect = Rect(0,0,1,1);

    int simDist = 32;
    double distX;
    double distY;
    vector<Point> dest_pts;
    int degrees2 = 0;
    int degrees3 = 0;
    int degrees4 = 0;
    Quadrilateral transformed;
    Quadrilateral quad1;
    double tan;
    if(savedQuad.area && masked) {

        Mat mask = Mat::zeros(tmp1.rows, tmp1.cols, CV_8U);
        double d45 = CV_PI / 4;

        if (!preview) {
            distX = distanceBtwPoints(savedQuad.p2, savedQuad.p3) * 1.33;
            distY = distanceBtwPoints(savedQuad.p1, savedQuad.p2) * 1.33;

            transformed = { Point((480 - savedQuad.p4.y) * 1.33,savedQuad.p4.x * 1.33),
                            Point((480 - savedQuad.p1.y) * 1.33,savedQuad.p1.x * 1.33),
                            Point((480 - savedQuad.p2.y) * 1.33,savedQuad.p2.x * 1.33),
                            Point((480 - savedQuad.p3.y) * 1.33,savedQuad.p3.x * 1.33),0 };

            vector<Point> pts{transformed.p1,transformed.p2,transformed.p3,transformed.p4};
            quad1 = sortPoints(pts);
            tan = -atan(Slope(quad1.p1.x, quad1.p1.y, quad1.p4.x, quad1.p4.y));
            degrees3 = radiansToDegrees(tan);

            dest_pts.push_back(Point(quad1.p1.x + simDist * cos(tan + d45), quad1.p1.y - simDist * sin(tan + d45)));
            dest_pts.push_back(Point(quad1.p2.x + simDist * sin(tan + d45), quad1.p2.y + simDist * cos(tan + d45)));
            dest_pts.push_back(Point(quad1.p3.x - simDist * cos(tan + d45), quad1.p3.y + simDist * sin(tan + d45)));
            dest_pts.push_back(Point(quad1.p4.x - simDist * sin(tan + d45), quad1.p4.y - simDist * cos(tan + d45)));
        } else {
            vector<Point> pts{savedQuad.p1,savedQuad.p2,savedQuad.p3,savedQuad.p4};
            quad1 = sortPoints(pts);
            tan = atan(Slope(quad1.p1.x, quad1.p1.y, quad1.p4.x, quad1.p4.y));
            degrees4 = radiansToDegrees(tan);
            distX = distanceBtwPoints(savedQuad.p1, savedQuad.p2);
            distY = distanceBtwPoints(savedQuad.p2, savedQuad.p3);
            dest_pts.push_back(Point(savedQuad.p1.x + simDist * sin(tan + d45), savedQuad.p1.y - simDist * cos(tan + d45)));
            dest_pts.push_back(Point(savedQuad.p2.x + simDist * cos(tan + d45), savedQuad.p2.y + simDist * sin(tan + d45)));
            dest_pts.push_back(Point(savedQuad.p3.x - simDist * sin(tan + d45), savedQuad.p3.y + simDist * cos(tan + d45)));
            dest_pts.push_back(Point(savedQuad.p4.x - simDist * cos(tan + d45), savedQuad.p4.y - simDist * sin(tan + d45)));
        }
        if(distX > 80 && distY > 80)
        {
            //vector<vector<Point>> pts = {dest_pts};
            //vector<vector<Point>> pts = { { savedQuad.p, savedQuad.p2, savedQuad.p3, savedQuad.p4 } };
            fillPoly(mask, dest_pts, Scalar(255));

            bitwise_not(mask, mask);
            bitwise_and(tmp2, mask, tmp2);
        }

    }


    vector<Line> lines;
    HoughLinesP(tmp2, lines, 1, CV_PI / 180, 110, 150, 22); //CV_PI/180
    //vector<Vec2f> lines;
    //HoughLines(tmp2, lines, 1, CV_PI / 180, 350, 0, 0); //CV_PI/180


    vector<Line> linesP = processLines(lines, MIN_MERGE_DISTANCE, MIN_MERGE_ANGLE);


    //vector<Line> linesP;
    if (!preview) {
        /*
        for (int i = 0; i < lines.size(); i++) {
            line(debug, BEGIN_POINT(lines[i]), END_POINT(lines[i]), Scalar(0, 255, 255), 1);
        }
         */
        /*
        line(tmp2, quad1.p1, quad1.p2, Scalar(0, 255, 0), 4);
        line(tmp2, quad1.p2, quad1.p3, Scalar(0, 255, 0), 4);
        line(tmp2, quad1.p3, quad1.p4, Scalar(0, 255, 0), 4);
        line(tmp2, quad1.p4, quad1.p1, Scalar(0, 255, 0), 4);
         */
    }

    intersections = findIntersections(linesP, gray.size());


    //return true;
    bool ret = filterOptions(intersections, outline);



    clock_gettime(CLOCK_BOOTTIME, &finish);
    elapsed = (finish.tv_sec - start.tv_sec);
    elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    total += elapsed;

    if(timerOn) {
        //LOGD("NEKI");
        startTimer = true;
        timeCount++;
        comb += total;
    }
    else {
        if(startTimer) {
            startTimer = false;
            incr++;
            avg = comb / (double)timeCount;
            LOGD("AVG_%d: %f\n", incr, avg);
        }

        timeCount = 0;
        comb = 0;
    }

    //LOGD("Total: %f\n", total);

    if(!preview) {
        if(ret) {
            savedQuad.area = 0;
            return ret;
        }
        if(preCounter < 5){
            outline = transformed;
            return true;
        }
    }
    if(ret) {
        savedQuad = outline;
        preCounter = 0;
    }
    else if(preCounter < 5) {
        preCounter++;
        outline = savedQuad;
        return true;
    }
    else {
        savedQuad.area = 0;
    }
    return ret;
}

vector<Point> getIntersections() {
    return intersections;
}

void warp_document(const Mat& src, Quadrilateral region, Mat& dst) {
    // todo do some checks on input.
    double distX = distanceBtwPoints(region.p2, region.p3);
    double distY = distanceBtwPoints(region.p1, region.p2);

    Point2f dest_pts[4];
    dest_pts[0] = Point2f(0, 0);
    dest_pts[1] = Point2f(distX, 0);
    dest_pts[2] = Point2f(distX,distY);
    dest_pts[3] = Point2f(0, distY);

    Point2f src_pts[4];
    src_pts[0] = region.p2;
    src_pts[1] = region.p3;
    src_pts[2] = region.p4;
    src_pts[3] = region.p1;

    Mat transform = cv::getPerspectiveTransform(src_pts, dest_pts);
    cv::warpPerspective(src, dst, transform, cv::Size((int)(dest_pts[1].x - dest_pts[0].x), (int) (dest_pts[3].y - dest_pts[0].y)));
}
