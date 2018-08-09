#include <algorithm>
#include "geo_lib.h"

void Geo::findViewLimitVertices(Point t, std::vector<Point> pol, int &i1, int &i2) {
    std::vector<BoundaryNode *> clone;
    for (int i = 0; i < pol.size(); i++) {
        BoundaryNode *b = new BoundaryNode();
        b->x_ = pol[i].x_;
        b->y_ = pol[i].y_;
        b->id_ = i;
        clone.push_back(b);
    }

    BoundaryNode pivot;
    pivot.x_ = t.x_;
    pivot.y_ = t.y_;
    std::sort(clone.begin(), clone.end(), POLAR_ORDER(pivot));
    i1 = clone[0]->id_;
    i2 = clone[clone.size() - 1]->id_;
}

double Geo::pathLength(std::vector<BasePathPoint> p) {
    double length = 0;
    if (p.size() == 0)
        return 0;
    for (int i = 0; i < p.size() - 1; i++) {
        length += G::distance(p[i], p[i + 1]);
    }
    return length;
}

double Geo::pathLength(std::vector<InsidePoint> p) {
    double length = 0;
    if (p.size() == 0)
        return 0;
    for (int i = 0; i < p.size() - 1; i++) {
        length += G::distance(p[i], p[i + 1]);
    }
    return length;
}

double Geo::pathLength(std::vector<Point> p) {
    double length = 0;
    if (p.size() == 0)
        return 0;
    for (int i = 0; i < p.size() - 1; i++) {
        length += G::distance(p[i], p[i + 1]);
    }
    return length;
}

bool Geo::isVertexOfPolygon(Point p, std::vector<BoundaryNode> pol, int &index) {
    for (int i = 0; i < pol.size(); i++) {
        if (pol[i] == p) {
            index = i;
            return true;
        }
    }
    return false;
}

bool Geo::isVertexOfPolygon(Point p, std::vector<BoundaryNode> pol) {
    for (int i = 0; i < pol.size(); i++) {
        if (fabs(pol[i].x_ - p.x_) < 0.00001 && fabs(pol[i].y_ - p.y_) < 0.00001) {
            return true;
        }
    }
    return false;
}

bool Geo::isVertexOfPolygon(Point p, std::vector<Point> pol) {
    for (int i = 0; i < pol.size(); i++) {
        if (fabs(pol[i].x_ - p.x_) < 0.00001 && fabs(pol[i].y_ - p.y_) < 0.00001) {
            return true;
        }
    }
    return false;
}

std::vector<CorePolygon> Geo::findObstacles(std::vector<CorePolygon> corePols, Point s, Point t) {
    // sap xep core polygon theo thu tu xa s dan
    std::vector<CorePolygon *> clone;
    for (int i = 0; i < corePols.size(); i++)
        clone.push_back(&corePols[i]);
    std::sort(clone.begin(), clone.end(), CORE_DIST_ORDER(s));

    // find obstacles
    std::vector<CorePolygon> result;

    // them 'core polygon' s va d vao
    CorePolygon tmp;
    tmp.vertices = std::vector<Point>(1, s);
    tmp.id = 0;
    result.push_back(tmp);

    for (int i = 0; i < clone.size(); i++) {
        if (G::segmentPolygonIntersect(s, t, clone[i]->vertices) ||
            (Geo::isVertexOfPolygon(s, clone[i]->vertices) && Geo::isVertexOfPolygon(t, clone[i]->vertices)))
            result.push_back(*clone[i]);
    }

    tmp.vertices = std::vector<Point>(1, t);
    tmp.id = result.size();
    result.push_back(tmp);

    return result;
}

bool Geo::isPointReallyInsidePolygon(Point p, std::vector<BoundaryNode> polygon) {
    bool odd = false;

    for (unsigned int i = 0; i < polygon.size(); i++) {
        Point tmp = polygon[i];
        Point tmp_next = polygon[(i + 1) % polygon.size()];
        if (G::is_in_line(tmp, tmp_next, p)) {
            if (G::onSegment(tmp, p, tmp_next)) {
                return false;
            }
        }
        if ((tmp.y_ > p.y_) != (tmp_next.y_ > p.y_) &&
            (p.x_ < (tmp_next.x_ - tmp.x_) * (p.y_ - tmp.y_) / (tmp_next.y_ - tmp.y_) + tmp.x_))
            odd = !odd;
    }
    return odd;
}

std::vector<Point> Geo::boundaryPath(Point a, Point b, std::vector<BoundaryNode> cg) {
    if (a == b)
        return std::vector<Point>();

    int a_, b_;
    isVertexOfPolygon(a, cg, a_);
    isVertexOfPolygon(b, cg, b_);

    std::vector<Point> path;

    int cg_size = cg.size();
    bool orient;
    boundaryLength(a, b, cg, orient);

    if (orient) {
        if (b_ < a_) b_ += cg_size;
        for (int i = a_; i <= b_; i++)
            path.push_back(Point(cg[i % cg_size].x_, cg[i % cg_size].y_));
    } else {
        if (a_ < b_) a_ += cg_size;
        for (int i = b_; i <= a_; i++)
            path.push_back(Point(cg[i % cg_size].x_, cg[i % cg_size].y_));
        std::reverse(path.begin(), path.end());
    }

    return path;
}

double Geo::boundaryLength(Point a, Point b, std::vector<BoundaryNode> cg, bool &orient) {
    int a_, b_;
    isVertexOfPolygon(a, cg, a_);
    isVertexOfPolygon(b, cg, b_);

    double length = 0;

    int cg_size = cg.size();

    if (b_ < a_) b_ += cg_size;

    for (int i = a_; i < b_; i++) {
        length += G::distance(cg[i % cg_size], cg[(i + 1) % cg_size]);
    }

    // choose the shorter boundary
    if (length < polygonPerimeter(cg) / 2) {
        orient = true;  // a->b
    } else {
        length = polygonPerimeter(cg) - length;
        orient = false;
    }

    return length;
}

bool Geo::isPointInsidePolygon(Point d, std::vector<BoundaryNode> p) {
    bool odd = false;
    for (unsigned int i = 0; i < p.size(); i++) {
        Point tmp = p[i];
        Point tmp_next = p[(i + 1) % p.size()];
        if (G::is_in_line(tmp, tmp_next, d)) {
            if (G::onSegment(tmp, d, tmp_next)) {
                return true;
            }
        }
        if ((tmp.y_ > d.y_) != (tmp_next.y_ > d.y_) &&
            (d.x_ < (tmp_next.x_ - tmp.x_) * (d.y_ - tmp.y_) / (tmp_next.y_ - tmp.y_) + tmp.x_))
            odd = !odd;
    }
    return odd;
}

double Geo::polygonPerimeter(std::vector<BoundaryNode> p) {
    double peri_ = 0;
    for (int i = 0; i < p.size(); i++) {
        int j = i == p.size() - 1 ? 0 : (i + 1);
        peri_ += G::distance(p[i], p[j]);
    }
    return peri_;
}

double Geo::polygonPerimeter(CorePolygon p) {
    double peri_ = 0;
    for (int i = 0; i < p.vertices.size(); i++) {
        int j = i == p.vertices.size() - 1 ? 0 : (i + 1);
        peri_ += G::distance(p.vertices[i], p.vertices[j]);
    }
    return peri_;
}

double Geo::polygonMaxEdge(CorePolygon p) {
    std::vector<Point> ver_ = p.vertices;
    double max_len_ = 0;
    for (int i = 0; i < ver_.size(); i++) {
        Point tmp1 = ver_[i % ver_.size()];
        Point tmp2 = ver_[(i+1) % ver_.size()];
        double dis = G::distance(tmp1, tmp2);
        max_len_ = max_len_ < dis ? dis : max_len_;
    }
    return max_len_;
}

bool Geo::isPointInsidePolygon(Point p, std::vector<InsidePoint> polygon) {
    bool odd = false;
    for (unsigned int i = 0; i < polygon.size(); i++) {
        Point tmp = polygon[i];
        Point tmp_next = polygon[(i + 1) % polygon.size()];
        if (G::is_in_line(tmp, tmp_next, p)) {
            if (G::onSegment(tmp, p, tmp_next)) {
                return true;
            }
        }
        if ((tmp.y_ > p.y_) != (tmp_next.y_ > p.y_) &&
            (p.x_ < (tmp_next.x_ - tmp.x_) * (p.y_ - tmp.y_) / (tmp_next.y_ - tmp.y_) + tmp.x_))
            odd = !odd;
    }
    return odd;
}


bool Geo::isSegmentInsidePolygon(InsidePoint s, InsidePoint d, std::vector<InsidePoint> p) {
    if (Geo::segmentPolygonIntersect(s, d, p))
        return false;
    else {
        InsidePoint m_;
        m_.x_ = G::midpoint(s, d).x_;
        m_.y_ = G::midpoint(s, d).y_;

        if (!Geo::isPointReallyInsidePolygon(m_, p))    // todo: cung chua chuan lam nhung tam on trong hau het cac truong hop can dung
            return false;                                           // todo: cung co the midpoint van la dinh cua polygon
        else return true;
    }
}


bool Geo::isPointReallyInsidePolygon(InsidePoint p, std::vector<InsidePoint> polygon) {
    bool odd = false;

    for (unsigned int i = 0; i < polygon.size(); i++) {
        Point tmp = polygon[i];
        Point tmp_next = polygon[(i + 1) % polygon.size()];
        if (G::is_in_line(tmp, tmp_next, p)) {
            if (G::onSegment(tmp, p, tmp_next)) {
                return false;
            }
        }
        if ((tmp.y_ > p.y_) != (tmp_next.y_ > p.y_) &&
            (p.x_ < (tmp_next.x_ - tmp.x_) * (p.y_ - tmp.y_) / (tmp_next.y_ - tmp.y_) + tmp.x_))
            odd = !odd;
    }
    return odd;
}

bool Geo::segmentPolygonIntersect(Point s, Point d, std::vector<BoundaryNode> p) {
    int num_intersection = 0;
    for (unsigned int i = 0; i < p.size(); i++) {
        int j = (i == p.size() - 1 ? 0 : (i + 1));
        if (G::is_intersect2(&s, &d, p[i], p[j]))
            num_intersection++;
    }
    return num_intersection > 0;
}


bool Geo::segmentPolygonIntersect(Point s, Point d, std::vector<InsidePoint> p) {
    int num_intersection = 0;
    for (unsigned int i = 0; i < p.size(); i++) {
        int j = (i == p.size() - 1 ? 0 : (i + 1));
        if (G::is_intersect2(&s, &d, p[i], p[j]))
            num_intersection++;
    }
    return num_intersection > 0;
}


void Geo::convexSetToCorePolygonSet(std::vector<std::vector<BoundaryNode>> cv_set_, std::vector<CorePolygon> &core_set_) {
    for (int i = 0; i < cv_set_.size(); i++) {
        CorePolygon tmp;
        for (int j = 0; j < cv_set_.at(i).size(); j++) {
            tmp.vertices.push_back(cv_set_.at(i).at(j));
        }
        core_set_.push_back(tmp);
    }
    return;
}

void Geo::corePolygonSetToConvexSet(std::vector<CorePolygon> core_set_, std::vector<std::vector<BoundaryNode>> &cv_set_) {
    for (int i = 0; i < core_set_.size(); i++) {
        std::vector<BoundaryNode> tmp;
        for (int j = 0; j < core_set_.at(i).vertices.size(); j++) {
            BoundaryNode n;
            n.x_ = core_set_.at(i).vertices.at(j).x_;
            n.y_ = core_set_.at(i).vertices.at(j).y_;
            //    n.is_convex_hull_boundary_ = true;
            tmp.push_back(n);
        }
        cv_set_.push_back(tmp);
    }
    return;
}

void Geo::convexToCorePolygon(std::vector<BoundaryNode> cv, CorePolygon &core) {
    for (int i = 0; i < cv.size(); i++)
        core.vertices.push_back(cv.at(i));
    return;
}

void Geo::corePolygonToConvex(CorePolygon core, std::vector<BoundaryNode> &cv) {
    for (int i = 0; i < core.vertices.size(); i++) {
        BoundaryNode n;
        n.x_ = core.vertices.at(i).x_;
        n.y_ = core.vertices.at(i).y_;
        n.is_convex_hull_boundary_ = true;
        cv.push_back(n);
    }
    return;
}