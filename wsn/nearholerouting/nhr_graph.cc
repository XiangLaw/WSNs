#include <wsn/graph/voronoi/VoronoiDiagram.h>
#include <queue>
#include <climits>
#include "nhr_graph.h"

using namespace std;

NHRGraph::NHRGraph(Point agent, std::vector<BoundaryNode> hole) {
    agent_ = agent;
    hole_ = hole;
    isPivot_ = true;
    endpoint_ = agent; // default endpoint is this node if it is not inside hole
    constructGraph();
}

void NHRGraph::constructGraph() {
    map<Point, std::vector<Point> > graph;
    int site_contains_node;
    Point gate_endpoint;
    set<Point> endpoints;

    // reconstruct caves & determine cave containing current node
    // NOTE: if the current node is the gate then return immediately
    for (unsigned int i = 0; i < hole_.size() - 1; i++) {
        if (hole_.at(i).is_convex_hull_boundary_ &&
            agent_.x_ == hole_.at(i).x_ && agent_.y_ == hole_.at(i).y_)
            return;
        if (hole_.at(i).is_convex_hull_boundary_ && !hole_.at(i + 1).is_convex_hull_boundary_) {
            cave_.push_back(hole_.at(i++));
            while (!hole_.at(i).is_convex_hull_boundary_) {
                cave_.push_back(hole_.at(i++));
            }
            cave_.push_back(hole_.at(i--));
            if (cave_.size() >= MIN_CAVE_VERTEX) {
                polygonHole *node_list = new polygonHole();
                node_list->node_list_ = NULL;
                node_list->next_ = NULL;
                for (unsigned int j = 0; j < cave_.size(); j++) {
                    node *tmp = new node();
                    tmp->x_ = cave_.at(j).x_;
                    tmp->y_ = cave_.at(j).y_;
                    tmp->next_ = node_list->node_list_;
                    node_list->node_list_ = tmp;
                }
                if (G::isPointInsidePolygon(&agent_, node_list->node_list_)) {
                    delete node_list;
                    break;
                }
                delete node_list;
            }
            std::vector<BoundaryNode>().swap(cave_);
        }
    }

    if (cave_.empty()) return;

    // 1. check if perpendicular line from current node to gate line intersects with any edge of polygon (except gate) or not
    Point perpendicular_point;
    bool no_voronoi = perpendicularLinePolygonIntersect(agent_, cave_, perpendicular_point);

    if (no_voronoi) {
        endpoint_ = perpendicular_point;
        return;
    }

    // 2. else find endpoint from voronoi graph
    isPivot_ = false;
    double min_distance = G::distance(agent_, cave_[0]);
    site_contains_node = 0;
    for (int i = 1; i < cave_.size(); i++) {
        double tmp_distance = G::distance(agent_, cave_[i]);
        if (min_distance > tmp_distance) {
            min_distance = tmp_distance;
            site_contains_node = cave_[i].id_;
        }
    }

    // calculate voronoi diagram
    std::vector<voronoi::VoronoiSite *> sites;

    for (int i = 0; i < cave_.size(); i++) {
        BoundaryNode p = cave_[i];
        sites.push_back(new voronoi::VoronoiSite(p.id_, p.x_, p.y_));
    }

    voronoi::VoronoiDiagram *diagram = voronoi::VoronoiDiagram::create(sites);

    // validate voronoi vertices (i.e. remove vertices outside the polygon)
    // find rectangle boundary of polygon
    double min_x, max_x, min_y, max_y;
    min_x = cave_[0].x_;
    max_x = cave_[0].x_;
    min_y = cave_[0].x_;
    max_y = cave_[0].x_;
    for (int i = 1; i < cave_.size(); i++) {
        if (cave_[i].x_ < min_x) {
            min_x = cave_[i].x_;
        } else if (cave_[i].x_ > max_x) {
            max_x = cave_[i].x_;
        }
        if (cave_[i].y_ < min_y) {
            min_y = cave_[i].y_;
        } else if (cave_[i].y_ > max_y) {
            max_y = cave_[i].y_;
        }
    }

    for (std::set<geometry::Point>::iterator it = diagram->vertices().begin(); it != diagram->vertices().end();) {
        Point tmp_point;
        tmp_point.x_ = (*it).x();
        tmp_point.y_ = (*it).y();
        if (!validateVoronoiVertex(tmp_point, cave_, min_x, max_x, min_y, max_y)) {
            diagram->vertices().erase(it++);
        } else {
            ++it;
        }
    }

    // construct graph
    for (std::vector<voronoi::VoronoiEdge *>::iterator it = diagram->edges().begin();
         it != diagram->edges().end(); ++it) {
        Point p1, p2;
        p1.x_ = (*it)->edge().startPoint().x();
        p1.y_ = (*it)->edge().startPoint().y();
        p2.x_ = (*it)->edge().endPoint().x();
        p2.y_ = (*it)->edge().endPoint().y();
        bool b1 = diagram->vertices().find((*it)->edge().startPoint()) != diagram->vertices().end();
        bool b2 = diagram->vertices().find((*it)->edge().endPoint()) != diagram->vertices().end();
        if ((*it)->leftSite().id() == site_contains_node || (*it)->rightSite().id() == site_contains_node) {
            if (b1)
                endpoints.insert(p1);
            if (b2)
                endpoints.insert(p2);
        }
        if (b1 && b2) {
            addVertexToGraph(graph, p1, p2);
        } else if (b1 || b2) { // determine gate point
            Line pp = G::line(p1, p2);
            Point ins;
            if (G::lineSegmentIntersection(&cave_[0], &cave_[cave_.size() - 1], pp, ins) &&
                G::onSegment(p1, ins, p2)) {
                addVertexToGraph(graph, b1 ? p1 : p2, ins);
                gate_endpoint.x_ = ins.x_;
                gate_endpoint.y_ = ins.y_;
            }
        }
    }
    delete diagram;
    gatepoint_ = gate_endpoint;
    endpoint_ = findShortestPath(graph, gate_endpoint, endpoints);
}

bool NHRGraph::validateVoronoiVertex(Point p, std::vector<BoundaryNode> polygon,
                                     double min_x, double max_x, double min_y, double max_y) {
    if (p.x_ < min_x || p.x_ > max_x || p.y_ < min_y || p.y_ > max_y)
        return false;

    bool odd = false;
    int i, j;

    for (i = 0, j = (int) polygon.size() - 1; i < polygon.size(); j = i++) {
        if (((polygon[i].y_ > p.y_) != (polygon[j].y_ > p.y_)) &&
            (p.x_ < (polygon[j].x_ - polygon[i].x_) * (p.y_ - polygon[i].y_) / (polygon[j].y_ - polygon[i].y_) +
                    polygon[i].x_))
            odd = !odd;
    }

    return odd;
}

void NHRGraph::addVertexToGraph(std::map<Point, std::vector<Point> > &graph, Point v1, Point v2) {
    std::map<Point, std::vector<Point> >::iterator it;
    it = graph.find(v1);
    if (it != graph.end()) {
        it->second.push_back(v2);
    } else {
        std::vector<Point> val;
        val.push_back(v2);
        graph.insert(std::pair<Point, std::vector<Point> >(v1, val));
    }
    it = graph.find(v2);
    if (it != graph.end()) {
        it->second.push_back(v1);
    } else {
        std::vector<Point> val;
        val.push_back(v1);
        graph.insert(std::pair<Point, std::vector<Point> >(v2, val));
    }
}

Point NHRGraph::findShortestPath(std::map<Point, std::vector<Point> > &graph, Point gate, set<Point> endpoints) {
    map<Point, Point> trace;
    std::queue<Point> queue;
    map<Point, int> level;
    int n_level = 0;
    std::map<Point, std::vector<Point> >::iterator root;
    root = graph.find(gate);
    queue.push(root->first);
    trace.insert(std::pair<Point, Point>(root->first, root->first));
    level.insert(std::pair<Point, int>(root->first, n_level));

    while (!queue.empty()) {
        n_level++;
        Point p = queue.front();
        queue.pop();
        std::vector<Point> tmp = graph.find(p)->second;
        for (std::vector<Point>::iterator it = tmp.begin(); it != tmp.end(); ++it) {
            if (trace.find((*it)) == trace.end()) {
                trace.insert(std::pair<Point, Point>((*it), p));
                level.insert(std::pair<Point, int>((*it), n_level));
                queue.push((*it));
            }
        }
    }

    Point closet_endpoint;
    int closet_level = INT_MAX;
    for (std::set<Point>::iterator it = endpoints.begin(); it != endpoints.end(); ++it) {
        int level_tmp = level.find(*it)->second;
        if (level_tmp < closet_level) {
            closet_level = level_tmp;
            closet_endpoint.x_ = (*it).x_;
            closet_endpoint.y_ = (*it).y_;
        }
    }

    Point tmp = closet_endpoint;
    while (tmp != gate) {
        std::map<Point, Point>::iterator it = trace.find(tmp);
        trace_.insert(std::pair<Point, Point>(it->first, it->second));
        tmp = it->second;
    }

    return closet_endpoint;
}

bool NHRGraph::perpendicularLinePolygonIntersect(Point p, std::vector<BoundaryNode> cave, Point &perpendicular_point) {
    Line gate_line = G::line(cave[0], cave[cave.size() - 1]);
    Line perpendicular_line = G::perpendicular_line(p, gate_line);
    G::intersection(gate_line, perpendicular_line, &perpendicular_point);
    for (int i = 0; i < cave.size() - 2; i++) {
        if (cave[i] == p || cave[i + 1] == p) continue;
        Point ins;
        if (G::lineSegmentIntersection(&cave[i], &cave[i + 1], perpendicular_line, ins) &&
            G::onSegment(&p, &ins, &perpendicular_point)) {
            return false;
        }
    }
    return true;
}

void NHRGraph::endpoint(Point &p) {
    p = trace_.find(p)->second;
}

Point NHRGraph::traceBack(Point &current_vertex) {
    map<Point, Point>::iterator it = trace_.find(current_vertex);
    if (it == trace_.end())
        return agent_;

    for (it = trace_.begin(); it != trace_.end(); it++) {
        if (it->second.operator==(it->first)) {
            continue;
        }
        if (it->second == current_vertex) {
            current_vertex = it->first;
            return it->first;
        }
    }
    return agent_;
}

void NHRGraph::dumpVoronoi(std::vector<BoundaryNode> polygon, std::map<Point, std::vector<Point> > vertices) {
    FILE *fp = fopen("Voronoi.tr", "a+");
    for (map<Point, std::vector<Point> >::iterator it = vertices.begin(); it != vertices.end(); ++it) {
        for (std::vector<Point>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            fprintf(fp, "==%d\t%f\t%f\n", 1, (*it).first.x_, (*it).first.y_);
            fprintf(fp, "==%d\t%f\t%f\n\n", 1, (*it2).x_, (*it2).y_);
        }
    }
    fclose(fp);
}
