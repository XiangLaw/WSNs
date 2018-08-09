#pragma once

#include <utility>
#include <list>
#include <vector>
#include <wsn/geomathhelper/geo_math_helper.h>
#include <wsn/versatilerouting_v1/geometry_library/geo_lib.h>

using namespace std;

typedef std::pair<double, int> pair_;

class VR2IGraph {
    std::list<pair_> *adj;
    std::vector<InsidePoint> polygon;
    std::vector<int> STP;       // shortest path - index in polygon of point in shortest path

    void addVertexToPath(std::vector<int> parent, int j);

    bool isVertexOfPolygon(InsidePoint s, std::vector<InsidePoint> p);

    int getIndexInPolygon(InsidePoint s, std::vector<InsidePoint> p);

public:

    VR2IGraph(std::vector<InsidePoint> hole);

    vector<InsidePoint> shortestPath(InsidePoint s, InsidePoint t);
};