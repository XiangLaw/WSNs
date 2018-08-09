#include "o_graph.h"
#include <functional>
#include <queue>
#include <iostream>
#include <algorithm>

// cg_set_ nay phai co phan tu dau tien la source, phan tu cuoi cung la dest
OGraph::OGraph(std::vector<CorePolygon> &cg_set_) {

    // save the core polygons set into memory
    for (unsigned int i = 0; i < cg_set_.size(); i++)
        core_set_.push_back(cg_set_[i]);


    // find tangents set between core polygons - add vertices to graph
    for (int i = 0; i < cg_set_.size() - 1; i++) {
        CorePolygon cg1 = cg_set_[i];
        CorePolygon cg2 = cg_set_[i + 1];
        for (int j = 0; j < cg1.vertices.size(); j++) {
            int j1, j2;
            Geo::findViewLimitVertices(cg1.vertices[j], cg2.vertices, j1, j2);
            if (j1 != j2) {
                if (!G::linePolygonIntersect(cg1.vertices[j], cg2.vertices[j1], cg1.vertices)
                    && !G::linePolygonIntersect(cg1.vertices[j], cg2.vertices[j1], cg2.vertices))
                    tagent_set_.push_back(Tangent(cg1.vertices[j], cg2.vertices[j1], i, i + 1));

                if (!G::linePolygonIntersect(cg1.vertices[j], cg2.vertices[j2], cg1.vertices)
                    && !G::linePolygonIntersect(cg1.vertices[j], cg2.vertices[j2], cg2.vertices))
                    tagent_set_.push_back(Tangent(cg1.vertices[j], cg2.vertices[j2], i, i + 1));
            } else {
                if (!G::linePolygonIntersect(cg1.vertices[j], cg2.vertices[j1], cg1.vertices)
                    && !G::linePolygonIntersect(cg1.vertices[j], cg2.vertices[j1], cg2.vertices))
                    tagent_set_.push_back(Tangent(cg1.vertices[j], cg2.vertices[j1], i, i + 1));
            }
        }
    }

    // initialize empty adjacent list
    adj = new list<pair_>[tagent_set_.size() + 2];


    // create adjacent list
    for (unsigned int i = 0; i < tagent_set_.size(); i++) {
        for (unsigned int j = i + 1; j < tagent_set_.size(); j++) {
            if (tagent_set_[j].start_hole_id_ == tagent_set_[i].end_hole_id_) {

                // calculate 'distance' between tagent_set_[i] & tagent_set_[j]
                int hole_id_ = tagent_set_[j].start_hole_id_;
                double dist =
                        boundaryLength(tagent_set_[i].end, tagent_set_[j].start, cg_set_[hole_id_]) +
                        tagent_set_[i].length / 2 +
                        tagent_set_[j].length / 2;

                // add new edge to graph - de tranh di vong lai thi chi can add canh theo 1 chieu tu s->d
                adj[i].push_back(make_pair(dist, j));
            }
        }
    }
}


double OGraph::boundaryLength(Point a, Point b, CorePolygon cg) {
    int a_ = getVertexIndexInPolygon(a, cg.vertices);
    int b_ = getVertexIndexInPolygon(b, cg.vertices);

    double length = 0;
    int cg_size = cg.vertices.size();
    if (b_ < a_) b_ += cg_size;

    for (int i = a_; i < b_; i++) {
        length += G::distance(cg.vertices[i % cg_size], cg.vertices[(i + 1) % cg_size]);
    }

    // choose the shorter boundary
    length = length < polygonPerimeter(cg) / 2 ? length : polygonPerimeter(cg) - length;
    return length;
}

double OGraph::boundaryLength(Point a, Point b, CorePolygon cg, bool &orient) {
    int a_ = getVertexIndexInPolygon(a, cg.vertices);
    int b_ = getVertexIndexInPolygon(b, cg.vertices);

    double length = 0;

    int cg_size = cg.vertices.size();

    if (b_ < a_) b_ += cg_size;

    for (int i = a_; i < b_; i++) {
        length += G::distance(cg.vertices[i % cg_size], cg.vertices[(i + 1) % cg_size]);
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

int OGraph::getVertexIndexInPolygon(Point t, std::vector<Point> p) {
    for (int i = 0; i < p.size(); i++) {
        if (t.x_ == p[i].x_ && t.y_ == p[i].y_)
            return i;
    }
    return -1;
}

double OGraph::polygonPerimeter(CorePolygon cg) {
    double peri = 0;
    for (int i = 0; i < cg.vertices.size(); i++) {
        int j = (i == cg.vertices.size() - 1) ? (i + 1) : 0;
        peri += G::distance(cg.vertices[i], cg.vertices[j]);
    }
    return peri;
}

std::vector<Point> OGraph::boundaryPath(Point a, Point b, CorePolygon cg) {
    if (a == b)
        return std::vector<Point>();

    int a_ = getVertexIndexInPolygon(a, cg.vertices);
    int b_ = getVertexIndexInPolygon(b, cg.vertices);

    std::vector<Point> path;

    int cg_size = cg.vertices.size();
    bool orient;
    boundaryLength(a, b, cg, orient);

    if (orient) {
        if (b_ < a_) b_ += cg_size;
        for (int i = a_; i <= b_; i++)
            path.push_back(Point(cg.vertices[i % cg_size].x_, cg.vertices[i % cg_size].y_));
    } else {
        if (a_ < b_) a_ += cg_size;
        for (int i = b_; i <= a_; i++)
            path.push_back(Point(cg.vertices[i % cg_size].x_, cg.vertices[i % cg_size].y_));
        std::reverse(path.begin(), path.end());
    }

    return path;
}

void OGraph::addVertexToPath(std::vector<int> parent, int j) {
    if (parent[j] != -1) {
        addVertexToPath(parent, parent[j]);
        STP.push_back(parent[j]);
    }
}

double OGraph::shortestPathLength() {
    findAllPaths();
    double min_length_ = DBL_MAX;
    for (int i = 0; i < all_paths_points.size(); i++)
    {
        double tmp = Geo::pathLength(all_paths_points[i]);
        min_length_ = min_length_ < tmp ? min_length_ : tmp;
    }
    return min_length_;
}

std::vector<BasePathPoint> OGraph::shortestPath() {
    findAllPaths();
    double min_length_ = DBL_MAX;
    int min_index_;

    for (int i = 0; i < all_paths_points.size(); i++)
    {
        double tmp = Geo::pathLength(all_paths_points[i]);
        if (tmp < min_length_)
        {
            min_length_ = tmp;
            min_index_ = i;
        }
    }

    return all_paths_points[min_index_];
}

std::vector<std::vector<BasePathPoint>> OGraph::basePaths(double epsilon, int n) {
    findCuteAnglePaths();
    double length = 0;
    double base_length = (1 + epsilon) * sin((n - 2) * M_PI / (2 * n)) * shortestPathLength();
    for (int i = 0; i < all_paths_points.size(); i++) {
        std::vector<BasePathPoint> tmp_path = all_paths_points[i];
        for (int j = 0; j < tmp_path.size() - 1; j++) {
            length += G::distance(tmp_path[j], tmp_path[j+1]);
        }
        if (length <= base_length)
            base_paths_.push_back(tmp_path);
        length = 0;
    }

    return base_paths_;
}

void OGraph::findAllPaths() {

    // find all path from s1 -> d1, s1 -> d2, s2 -> d1, s2 -> d2
    findAllPaths(0, tagent_set_.size() - 2);
    findAllPaths(0, tagent_set_.size() - 1);
    findAllPaths(1, tagent_set_.size() - 2);
    findAllPaths(1, tagent_set_.size() - 1);

    for (int i = 0; i < all_paths_.size(); i++)
    {
        std::vector<BasePathPoint> single_path_points_;
        for (int j = 0; j < all_paths_[i].size(); j++) {
            // 1. add 2 endpoint cua tangent vao
            BasePathPoint n1;
            int index = all_paths_[i][j];
            n1.x_ = tagent_set_[index].start.x_;
            n1.y_ = tagent_set_[index].start.y_;
            n1.hole_id_ = tagent_set_[index].start_hole_id_;
            single_path_points_.push_back(n1);

            BasePathPoint n2;
            n2.x_ = tagent_set_[index].end.x_;
            n2.y_ = tagent_set_[index].end.y_;
            n2.hole_id_ = tagent_set_[index].end_hole_id_;

            if (j == (all_paths_[i].size() - 1))
                single_path_points_.push_back(n2);

            if (j != (all_paths_[i].size() - 1)) {
                // 2. add them doan nam tren core polygon's boundary vao
                Point n3;
                index = all_paths_[i][j+1];
                n3.x_ = tagent_set_[index].start.x_;
                n3.y_ = tagent_set_[index].start.y_;
                std::vector<Point> core_boundary_path_ = boundaryPath(n2, n3, core_set_[n2.hole_id_]);
                if (!core_boundary_path_.empty())
                {
                    std::vector<BasePathPoint> bpp;
                    for (int k = 0; k < core_boundary_path_.size(); k++)
                    {
                        BasePathPoint tmp;
                        tmp.x_ = core_boundary_path_[k].x_;
                        tmp.y_ = core_boundary_path_[k].y_;
                        tmp.hole_id_ = n2.hole_id_;
                        bpp.push_back(tmp);
                    }
                    bpp.erase(bpp.end());
                    single_path_points_.insert(single_path_points_.end(), bpp.begin(), bpp.end());
                }
            }
        }
        all_paths_points.push_back(single_path_points_);
    }
}

void OGraph::addVertexToPath(int u, int d, bool *visited, int *path, int &path_index_) {
    visited[u] = true;
    path[path_index_] = u;
    path_index_++;

    std::vector<int> single_path_;

    if (u == d) {
        for (int i = 0; i < path_index_; i++)
            single_path_.push_back(path[i]);
        all_paths_.push_back(single_path_);
    }
    else
    {
        std::list<pair_>::iterator i;
        for (i = adj[u].begin(); i != adj[u].end(); i++) {
            if (!visited[(*i).second])
                addVertexToPath((*i).second, d, visited, path, path_index_);
        }
    }

    path_index_--;
    visited[u] = false;
}

void OGraph::findAllPaths(int s, int d) {
    int V = tagent_set_.size();
    bool *visited = new bool[V];
    int *path = new int[V];
    int path_index_ = 0;

    for (int i = 0; i < V; i++)
        visited[i] = false;

    addVertexToPath(s, d, visited, path, path_index_);
}

void OGraph::findCuteAnglePaths() {
    findAllPaths();
    for (int i = 0; i < all_paths_points.size(); i++)
    {
        if (!isAllAcuteAngle(all_paths_points[i]))
            all_paths_points.erase(all_paths_points.begin() + i);
    }
}

bool OGraph::isAllAcuteAngle(std::vector<BasePathPoint> path) {
    Point s = path[0];
    Point d = path[path.size() - 1];

    for (int i = 0; i < path.size() - 1; i++)
    {
        if (G::angle(path[i], path[i+1], s, d) > M_PI)
            return false;
    }
    return true;
}