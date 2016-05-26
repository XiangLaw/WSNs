#pragma once

#include <vector>
#include <map>
#include <wsn/geomathhelper/geo_math_helper.h>
#include "nhr.h"

using namespace std;

class NHRGraph {
public:
    NHRGraph(Point agent, vector<BoundaryNode> hole);

    Point endpoint() { return endpoint_; }

    void endpoint(Point &);

    Point gatePoint() { return gatepoint_; }

    bool isPivot() { return isPivot_; }

    Point traceBack(Point &);

    void getGateNodeIds(int &, int &);

    void dumpVoronoi(vector<BoundaryNode> polygon, map<Point, vector<Point> > vertices);


private:
    Point agent_;
    Point endpoint_;
    Point gatepoint_;
    bool isPivot_;
    map<Point, Point> trace_;
    vector<BoundaryNode> hole_;
    vector<BoundaryNode> cave_;

    void constructGraph();

    bool validateVoronoiVertex(Point, vector<BoundaryNode>, double, double, double, double);

    void addVertexToGraph(std::map<Point, vector<Point> > &, Point, Point);

    Point findShortestPath(std::map<Point, vector<Point> > &, Point, set<Point>);

    bool perpendicularLinePolygonIntersect(Point, vector<BoundaryNode>, Point &);
};