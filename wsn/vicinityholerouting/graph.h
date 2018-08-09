#pragma once

#include <wsn/geomathhelper/geo_math_helper.h>
#include <list>

using namespace std;

typedef std::pair<double, int> pair_;

class Graph {
    std::list<pair_> *adj;
    std::vector<Point> polygon;
    std::vector<int> STP;
    void addVertexToPath(std::vector<int> parent, int j);
    bool isVertexOfPolygon(Point s, std::vector<Point> p);
    int getIndexInPolygon(Point s, std::vector<Point> p);
public:
    Graph(std::vector<Point> hole);
    std::vector<Point> shortestPath(Point s, Point t);
};