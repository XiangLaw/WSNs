#pragma once

#include <wsn/geomathhelper/geo_math_helper.h>

class Geo;

struct BoundaryNode : Point {
    int id_;
    bool is_convex_hull_boundary_;

    // comparison is done first on y coordinate and then on x coordinate
    bool operator<(BoundaryNode n2) {
        return y_ == n2.y_ ? x_ < n2.x_ : y_ < n2.y_;
    }

    BoundaryNode(double x, double y) {
        Point(x, y);
    }
    BoundaryNode() {}
};

struct BasePathPoint: Point {
    int hole_id_;
};

struct InsidePoint: Point {
    int index_;

    InsidePoint() {}

    InsidePoint(double x, double y) {
        Point(x, y);
        index_ = 0;
    }

    InsidePoint(Point a, int ind_) {
        Point(a.x_, a.y_);
        index_ = ind_;
    }
};

// Hole struct
struct HoleSt {
    std::vector<BoundaryNode> node_vector_; // node vector of hole
    int id_;   // id of hole
};

struct SubPath {
    Point scale_center_;
    double scale_ratio_;
    std::vector<Point> sub_path_;
};

struct AllPath{
    int s_core_gate_index_;
    int d_core_gate_index_;
    std::vector<BasePathPoint> outside_path_;
    double len;
};

struct FinalPath{
    int s_inside_stp_index;
    int d_inside_stp_index;
    int outside_stp_index;
    double len;
};

// core polygon
struct CorePolygon {
    std::vector<Point> vertices;
    int id;
};

struct Tangent {
    Point start;
    Point end;
    double length;
    int start_hole_id_;
    int end_hole_id_;

    Tangent(Point a, Point b) {
        start = a;
        end = b;
        length = G::distance(a, b);
    }

    Tangent(Point a, Point b, int start_id_, int end_id_) {
        start = a;
        end = b;
        length = G::distance(a, b);
        start_hole_id_ = start_id_;
        end_hole_id_ = end_id_;
    }
};

struct COORDINATE_ORDER {
    bool operator()(const BoundaryNode *a, const BoundaryNode *b) const {
        return a->y_ == b->y_ ? a->x_ < b->x_ : a->y_ < b->y_;
    }
    bool operator()(const node *a, const node *b) const {
        return a->y_ == b->y_ ? a->x_ < b->x_ : a->y_ < b->y_;
    }

};

// used for sorting points according to polar order w.r.t the pivot
struct POLAR_ORDER {
    POLAR_ORDER(struct BoundaryNode p) { this->pivot = p; }

    bool operator()(const BoundaryNode *a, const BoundaryNode *b) const {
        int order = G::orientation(pivot, *a, *b);
        if (order == 0)
            return G::distance(pivot, *a) < G::distance(pivot, *b);
        return (order == 2);
    }

    struct BoundaryNode pivot;
};

// used for sorting corepolygons according to distance from source
struct CORE_DIST_ORDER {
    CORE_DIST_ORDER(Point p) { this->pivot = p; }
    bool operator()(const CorePolygon *a, const CorePolygon *b) const {
        return G::distance(a->vertices[0], pivot) < G::distance(b->vertices[0], pivot) ? 1 : 0;
    }
    Point pivot;
};

class Geo {
public:
    static void findViewLimitVertices(Point, std::vector<Point>, int &i1, int &i2);
    static double pathLength(std::vector<BasePathPoint> );
    static double pathLength(std::vector<Point>);
    static double pathLength(std::vector<InsidePoint>);
    static std::vector<CorePolygon> findObstacles(std::vector<CorePolygon>, Point, Point);
    static bool isPointInsidePolygon(Point, std::vector<BoundaryNode>);
    static bool isPointReallyInsidePolygon(Point, std::vector<BoundaryNode> );
    static double polygonPerimeter(std::vector<BoundaryNode> );
    static double polygonPerimeter(CorePolygon );
    static double polygonMaxEdge(CorePolygon);
    static bool isVertexOfPolygon(Point, std::vector<BoundaryNode>, int &);
    static bool isVertexOfPolygon(Point, std::vector<BoundaryNode>);
    static bool isVertexOfPolygon(Point, std::vector<Point>);
    static std::vector<Point> boundaryPath(Point a, Point b, std::vector<BoundaryNode> cg);
    static double boundaryLength(Point a, Point b, std::vector<BoundaryNode> cg, bool &orient);
    static bool isPointInsidePolygon(Point, std::vector<InsidePoint>);
    static bool isSegmentInsidePolygon(InsidePoint s, InsidePoint d, std::vector<InsidePoint> p);
    static bool isPointReallyInsidePolygon(InsidePoint p, std::vector<InsidePoint> polygon);
    static bool segmentPolygonIntersect(Point s, Point d, std::vector<BoundaryNode> p);
    static bool segmentPolygonIntersect(Point s, Point d, std::vector<InsidePoint> p);
    static void convexSetToCorePolygonSet(std::vector<std::vector<BoundaryNode>>, std::vector<CorePolygon> &);
    static void corePolygonSetToConvexSet(std::vector<CorePolygon>, std::vector<std::vector<BoundaryNode>> &);
    static void corePolygonToConvex(CorePolygon, std::vector<BoundaryNode> &);
    static void convexToCorePolygon(std::vector<BoundaryNode>, CorePolygon &);

};