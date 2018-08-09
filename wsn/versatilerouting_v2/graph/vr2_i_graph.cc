#include <functional>
#include <queue>
#include "vr2_i_graph.h"

VR2IGraph::VR2IGraph(std::vector<InsidePoint> polygon) {
    adj = new list<pair_>[polygon.size() + 2];

    for (unsigned int i = 0; i < polygon.size(); i++) {
        this->polygon.push_back(polygon[i]);
        for (unsigned int j = i + 1; j < polygon.size(); j++) {
            if (Geo::isSegmentInsidePolygon(polygon[i], polygon[j], polygon) ||
                j == i + 1) { // connect all segment that
                // doesn't cut polygon
                adj[i].push_back(make_pair(G::distance(polygon[i], polygon[j]), j));
                adj[j].push_back(make_pair(G::distance(polygon[i], polygon[j]), i));
            }
        }
    }
}

void VR2IGraph::addVertexToPath(std::vector<int> parent, int j) {
    if (parent[j] != -1) {
        addVertexToPath(parent, parent[j]);
        STP.push_back(parent[j]);
    }
}

bool VR2IGraph::isVertexOfPolygon(InsidePoint s, std::vector<InsidePoint> p) {
    for (auto it : p) {
        if (it.x_ == s.x_ && it.y_ == s.y_)
            return true;
    }
    return false;
}

int VR2IGraph::getIndexInPolygon(InsidePoint s, std::vector<InsidePoint> p) {
    for (unsigned int i = 0; i < p.size(); i++) {
        if (p[i].x_ == s.x_ && p[i].y_ == s.y_)
            return i;
    }
    return NAN;
}

std::vector<InsidePoint> VR2IGraph::shortestPath(InsidePoint s, InsidePoint t) {
    std::vector<InsidePoint> new_polygon;
    for (auto it : polygon) new_polygon.push_back(it);

    // add s & t to polygon
    if (!isVertexOfPolygon(s, polygon))
        new_polygon.push_back(s);
    if (!isVertexOfPolygon(t, polygon))
        new_polygon.push_back(t);
    unsigned int s_ = getIndexInPolygon(s, new_polygon);
    unsigned int t_ = getIndexInPolygon(t, new_polygon);

    if (!isVertexOfPolygon(s, polygon))
        for (unsigned int i = 0; i < new_polygon.size(); i++) {
            if (i != s_)
                if (Geo::isSegmentInsidePolygon(new_polygon[i], s, polygon)) {
                    adj[i].push_back(make_pair(G::distance(new_polygon[i], s), s_));
                    adj[s_].push_back(make_pair(G::distance(new_polygon[i], s), i));
                }
        }

    if (!isVertexOfPolygon(t, polygon))
        for (unsigned int j = 0; j < new_polygon.size(), j != t_; j++) {
            if (j != t_)
                if (Geo::isSegmentInsidePolygon(new_polygon[j], t, polygon)) {
                    adj[j].push_back(make_pair(G::distance(new_polygon[j], t), t_));
                    adj[t_].push_back(make_pair(G::distance(new_polygon[j], t), j));
                }
        }

    std::priority_queue<pair_, std::vector<pair_>, greater<pair_>> pq;
    std::vector<double> dist(new_polygon.size(), DBL_MAX);
    std::vector<int> parent(new_polygon.size(), -1);

    pq.push(make_pair(0, s_));
    dist[s_] = 0;

    while (!pq.empty()) {
        int u = pq.top().second;
        pq.pop();

        list<pair_>::iterator it;
        for (it = adj[u].begin(); it != adj[u].end(); it++) {
            int v = (*it).second;
            double w = (*it).first;

            if (dist[v] > dist[u] + w) {
                dist[v] = dist[u] + w;
                pq.push(make_pair(dist[v], v));
                parent[v] = u;
            }
        }
    }

    addVertexToPath(parent, t_);
    STP.push_back(t_);
    std::vector<InsidePoint> sp;
    for (auto it : STP)
        sp.push_back(new_polygon[it]);
    return sp;
}