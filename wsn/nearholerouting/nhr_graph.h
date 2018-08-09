#pragma once

#include <vector>
#include <map>
#include <wsn/geomathhelper/geo_math_helper.h>
#include "nhr.h"

using namespace std;

class NHRGraph {
public:
    NHRGraph(Point agent, std::vector<BoundaryNode> hole);

    Point endpoint() { return endpoint_; }

    void endpoint(Point &);

    Point gatePoint() { return gatepoint_; }

    bool isPivot() { return isPivot_; }

    Point traceBack(Point &);

    void dumpVoronoi(std::vector<BoundaryNode> polygon, map<Point, std::vector<Point> > vertices);


private:
    Point agent_;
    Point endpoint_;
    Point gatepoint_;
    bool isPivot_;
    map<Point, Point> trace_;
    std::vector<BoundaryNode> hole_;
    std::vector<BoundaryNode> cave_;

    void constructGraph();

    bool validateVoronoiVertex(Point, std::vector<BoundaryNode>, double, double, double, double);

    void addVertexToGraph(std::map<Point, std::vector<Point> > &, Point, Point);

    Point findShortestPath(std::map<Point, std::vector<Point> > &, Point, set<Point>);

    bool perpendicularLinePolygonIntersect(Point, std::vector<BoundaryNode>, Point &);
};