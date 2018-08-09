#pragma once

#include <utility>
#include <list>
#include <vector>
#include <wsn/geomathhelper/geo_math_helper.h>
#include <wsn/versatilerouting_v1/geometry_library/geo_lib.h>

using namespace std;

typedef std::pair<double, int> pair_;

class GoalIGraph {
    std::list<pair_> *adj;
    std::vector<Point> polygon;
    std::vector<int> STP;       // shortest path - index in polygon of point in shortest path

    void addVertexToPath(std::vector<int> parent, int j);

    bool isVertexOfPolygon(Point s, std::vector<Point> p);

    int getIndexInPolygon(Point s, std::vector<Point> p);

public:

    GoalIGraph(std::vector<Point> hole);

    vector<Point> shortestPath(Point s, Point t);
};