/*
 * note:
 * 1. co the offset cua hdr bi tinh sai
 */

#include <packet.h>
#include <stack>
#include <algorithm>
#include "vhr.h"
#include "vhr_packet.h"
#include "vhr_packet_data.h"
#include "graph.h"

int hdr_vhr::offset_;

static class VHRHeaderClass : public PacketHeaderClass {
public:
    VHRHeaderClass() : PacketHeaderClass("PacketHeader/VHR", sizeof(hdr_vhr)) {
        bind_offset(&hdr_vhr::offset_);
    }
} class_vhrhdr;

static class VHRAgentClass : public TclClass {
public:
    VHRAgentClass() : TclClass("Agent/VHR") {}

    TclObject *create(int, const char *const *) {
        return (new VHRAgent());
    }
} class_vhr;

/*
 * Agent
 */
VHRAgent::VHRAgent() : BoundHoleAgent(), broadcast_timer_(this) {

    bind("broadcastAngle", &broadcastAngle);
    bind("scaleFactor", &scaleFactor);

    FILE *fp;
    fp = fopen("Neighbors.tr", "w");
    fclose(fp);
    fp = fopen("BoundHole.tr", "w");
    fclose(fp);
    fp = fopen("BroadcastRegion.tr", "w");
    fclose(fp);
    fp = fopen("TraceVHR.tr", "w");
    fclose(fp);
}

void VHRAgent::recv(Packet *p, Handler *h) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_ip *iph = HDR_IP(p);

    switch (cmh->ptype()) {
        case PT_VHR:
            recvHCI(p);
            break;
        case PT_CBR:
            if (iph->saddr() == my_id_)     // a packet generate by myself
            {
                if (cmh->num_forwards() == 0)   // a new packet
                {
                    sendData(p);
                }
                else    // (cmh->num_forwards() > 0) // routing loop -> drop
                {
                    drop(p, DROP_RTR_ROUTE_LOOP);
                    return;
                }
            }

            if (iph->ttl_ -- <= 0) {
                drop(p, DROP_RTR_TTL);
                return;
            }
            recvData(p);
            break;
        default:
        BoundHoleAgent::recv(p, h);
            break;
    }
}

int VHRAgent::command(int argc, const char *const *argv) {
    return BoundHoleAgent::command(argc, argv);
}

void VHRAgent::createHole(Packet *p) {
    int i = 0;
    std::vector<BoundaryNode> convex;

    polygonHole *h = createPolygonHole(p);
    for (struct node *node_tmp = h->node_list_; node_tmp; node_tmp = node_tmp->next_) {
       BoundaryNode bn;
        bn.x_ = node_tmp->x_;
        bn.y_ = node_tmp->y_;
        bn.is_convex_hull_boundary_ = false;
        bn.id_ = node_tmp->id_;
        hole.push_back(bn);
    }
    delete h;
}

std::vector<BoundaryNode> VHRAgent::determineConvexHull() {
    std::stack<BoundaryNode *> hull;
    std::vector<BoundaryNode> convex;
    std::vector<BoundaryNode *> clone_hole;

    for (std::vector<BoundaryNode>::iterator it = hole.begin(); it != hole.end(); ++it) {
        clone_hole.push_back(&(*it));
    }

    std::sort(clone_hole.begin(), clone_hole.end(), COORDINATE_ORDER());
    BoundaryNode *pivot = clone_hole.front();
    std::sort(clone_hole.begin() + 1, clone_hole.end(), POLAR_ORDER(*pivot));
    hull.push(clone_hole[0]);
    hull.push(clone_hole[1]);
    hull.push(clone_hole[2]);

    for (int i = 3; i < clone_hole.size(); i++) {
        BoundaryNode *top = hull.top();
        hull.pop();
        while (G::orientation(*hull.top(), *top, *clone_hole[i]) != 2) {
            top = hull.top();
            hull.pop();
        }
        hull.push(top);
        hull.push(clone_hole[i]);
    }

    while (!hull.empty()) {
        BoundaryNode *top = hull.top();
        top->is_convex_hull_boundary_ = true;
        convex.push_back(*top);
        hull.pop();
    }

    return convex;
}

/*
 *  Broadcast phase
 */
void VHRAgent::broadcastHCI() {
    Packet *p = NULL;
    VHRPacketData *payload;
    hdr_cmn *cmh;
    hdr_ip *iph;
    hdr_vhr *vhh;

    if (hole.empty())
        return;

    p = allocpkt();
    payload = new VHRPacketData();
    for (std::vector<BoundaryNode>::iterator it = hole.begin(); it != hole.end(); it++) {
        payload->add((*it).id_, (*it).x_, (*it).y_, (*it).is_convex_hull_boundary_);
    }
    p->setdata(payload);

    cmh = HDR_CMN(p);
    iph = HDR_IP(p);
    vhh = HDR_VHR(p);

    cmh->ptype() = PT_VHR;
    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = IP_BROADCAST;
    cmh->last_hop_ = my_id_;
    cmh->addr_type_ = NS_AF_INET;
    cmh->size() += IP_HDR_LEN + vhh->size();

    iph->daddr() = IP_BROADCAST;
    iph->saddr() = my_id_;
    iph->sport() = RT_PORT;
    iph->dport() = RT_PORT;
    iph->ttl_ = 4 * IP_DEF_TTL;

    send(p, 0);
}

void VHRAgent::recvHCI(Packet *p) {
    struct hdr_ip *iph = HDR_IP(p);

    // if the hci packet has came back to the initial node
    if (iph->saddr() == my_id_) {
        drop(p, "LoopHCI");
        return;
    }
    if (iph->ttl_-- <= 0) {
        drop(p, DROP_RTR_TTL);
        return;
    }

    // check if this node has the hole's info
    if (!hole.empty()) {
        drop(p, "HciReceived");
        return;
    }

    // store hci
    VHRPacketData *data = (VHRPacketData *) p->userdata();
    for (int i = 1; i <= data->size(); i++) {
        BoundaryNode n = data->get_data(i);
        hole.push_back(n);
    }

    if (!canBroadcast()) {
        drop(p, "OutsideBroadcastRegion");
    }
    else {
        broadcast_timer_.setParameter(p);
        broadcast_timer_.resched(randSend_.uniform(0.0, 1.0));
    }

//    dumpBroadcastRegion();
}

// check if viewangle from this node to convex hull >= 2pi/5
bool VHRAgent::canBroadcast() {
    std::vector<BoundaryNode> convex = determineConvexHull();
    if (findViewAngle(*this, convex) >= 2 * M_PI / 5)
        return true;
    return false;
}

double VHRAgent::findViewAngle(Point point, std::vector<BoundaryNode> convex) {
    int i1, i2;
    findViewLimitVertices(point, convex, i1, i2);
    return G::angle(point, &convex[i1], &convex[i2]);
}

void VHRAgent::findViewLimitVertices(Point point, std::vector<BoundaryNode> polygon, int &i1, int &i2) {
    std::vector<BoundaryNode *> clone;
    for (int i = 0; i < polygon.size(); i++) {
        polygon[i].id_ = i;
        clone.push_back(&polygon[i]);
    }

    BoundaryNode pivot;
    pivot.x_ = point.x_;
    pivot.y_ = point.y_;
    std::sort(clone.begin(), clone.end(), POLAR_ORDER(pivot));
    i1 = clone[0]->id_;
    i2 = clone[clone.size() - 1]->id_;
}

/*
 * Sending - Receiving data
 */
void VHRAgent::recvData(Packet *p) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_vhr *edh = HDR_VHR(p);

    if (cmh->direction() == hdr_cmn::UP && edh->dest == *this) {    // up to destination
        port_dmux_->recv(p, 0);
        return;
    }

    node *nexthop = NULL;

    int pth_len = getPathArraySize(edh->path);
    if (pth_len > 0) {
        nexthop = getNeighborByGreedy(edh->path[edh->apIndex]);
        while (nexthop == NULL && edh->apIndex < pth_len - 1) {
            edh->apIndex += 1;
            nexthop = getNeighborByGreedy(edh->path[edh->apIndex]);
        }
    } else {
        if (hole.empty()) {
            nexthop = getNeighborByGreedy(edh->dest);
        } else {
            std::vector<Point> path = findPath(*this, edh->dest);
            if (path.empty())
                return;
            for (int i = 0; i < path.size(); i++)
                edh->path[i] = path.at(i);
            edh->apIndex = 0;
            nexthop = getNeighborByGreedy(edh->path[edh->apIndex]);
        }
    }

    if (nexthop == NULL) {
        drop(p, DROP_RTR_NO_ROUTE);
        return;
    } else {
        cmh->direction() = hdr_cmn::DOWN;
        cmh->addr_type() = NS_AF_INET;
        cmh->last_hop_ = my_id_;
        cmh->next_hop_ = nexthop->id_;
        send(p, 0);
    }
}

// sendData just happen in source node
void VHRAgent::sendData(Packet *p) {
    hdr_cmn *cmh = HDR_CMN(p);
    hdr_ip *iph = HDR_IP(p);
    hdr_vhr *edh = HDR_VHR(p);

    cmh->size() += IP_HDR_LEN + edh->size();
    cmh->direction_ = hdr_cmn::DOWN;

    edh->apIndex = 0;
    edh->dest = *dest;
    if (!hole.empty()) {
        std::vector<Point> path = findPath(*this, *dest);
        if (path.empty())
            return;
        for (int i = 0; i < path.size(); i++)
            edh->path[i] = path.at(i);
    }
    iph->saddr() = my_id_;
    iph->daddr() = -1;
    iph->ttl_ = 6 * IP_DEF_TTL;
}

std::vector<Point> VHRAgent::findPath(Point s, Point d) {
    Point I; // homothetic center
    double k; // scale ratio
    std::vector<Point> path; // after scaling
    int s1, d1, s2, d2;
// 1. Determine convex hull
    std::vector<BoundaryNode> convex = determineConvexHull();
    std::vector<Point> p;
    for (auto it : convex)
        p.push_back(Point(it.x_, it.y_));

// Case 1: s, d are both outside convex hull
    if (!G::isPointInsidePolygon(s, p) && !G::isPointInsidePolygon(d, p)) {
        findViewLimitVertices(s, convex, s1, s2);
        findViewLimitVertices(d, convex, d1, d2);
        std::vector<Point> sp = bypassHole(G::distance(s, p[s1]), G::distance(s, p[s2]),
                                      G::distance(d, p[d1]), G::distance(d, p[d2]), s1, s2, d1,
                                      d2, p);

        // scaling
        std::vector<Point> sp_;
        sp_.push_back(s);
        sp_.insert(sp_.end(), sp.begin(), sp.end());
        sp_.push_back(d);
        I = findHomotheticCenter(sp_, scaleFactor, k);
        for (auto it : sp) {
            Point anchor = Point(k*it.x_ + (1-k)*I.x_, k*it.y_ + (1-k)*I.y_);
            path.push_back(anchor);
        }
        path.push_back(d);
        return path;
    }

// Case 2: s is outside convex hull, d is inside convex hull
    else if (!G::isPointInsidePolygon(s, p) && G::isPointInsidePolygon(d, p)) {
        findViewLimitVertices(s, convex, s1, s2);

        std::vector<Point> d_cave = determineCaveContainingNode(d);
        if (d_cave.empty())
            return path;
        Graph *graph1 = new Graph(d_cave);
        std::vector<Point> sp1 = graph1->shortestPath(d_cave[0], d);
        Graph *graph2 = new Graph(d_cave);
        std::vector<Point> sp2 = graph2->shortestPath(d_cave[d_cave.size() - 1],
                                                 d);

        delete (graph1);
        delete (graph2);

        std::vector<Point> sp = bypassHole(G::distance(s, p[s1]),
                                      G::distance(s, p[s2]), calculatePathLength(sp1),
                                      calculatePathLength(sp2), s1, s2,
                                      getVertexIndexInPolygon(d_cave[0], p),
                                      getVertexIndexInPolygon(d_cave[d_cave.size() - 1], p), p);

        // scale sp
        std::vector<Point> sp_;
        sp_.push_back(s);
        sp_.insert(sp_.end(), sp.begin(), sp.end());
        sp_.push_back(d);
        I = findHomotheticCenter(sp_, scaleFactor, k);
        for (unsigned int i = 0; i < sp.size() - 1; i++) {
            Point anchor = Point(k * sp[i].x_ + (1 - k) * I.x_, k * sp[i].y_ + (1 - k) * I.y_);
            path.push_back(anchor);
        }

        // scale concave path
        if (sp.back() == sp1.front()) {
            if (sp1.size() == 2) {
                path.push_back(sp1.front());
            } else {
                I = findHomotheticCenter(sp1, scaleFactor, k);
                for (unsigned int i = 0; i < sp1.size() - 1; i++) {
                    Point anchor = Point(k * sp1[i].x_ + (1 - k) * I.x_, k * sp1[i].y_ + (1 - k) * I.y_);
                    path.push_back(anchor);
                }
            }
        } else if (sp.back() == sp2.front()) {
            if (sp2.size() == 2) {
                path.push_back(sp2.front());
            } else {
                I = findHomotheticCenter(sp2, scaleFactor, k);
                for (unsigned int i = 0; i < sp2.size() - 1; i++) {
                    Point anchor = Point(k * sp2[i].x_ + (1 - k) * I.x_, k * sp2[i].y_ + (1 - k) * I.y_);
                    path.push_back(anchor);
                }
            }
        }
        path.push_back(d);
        return path;
    }

// Case 3: s is inside convex hull, d is outside convex hull
    else if (G::isPointInsidePolygon(s, p) && !G::isPointInsidePolygon(d, p)) {
        findViewLimitVertices(d, convex, d1, d2);
        std::vector<Point> s_cave = determineCaveContainingNode(s);
        if (s_cave.empty())
            return path;
        Graph *graph1 = new Graph(s_cave);
        std::vector<Point> sp1 = graph1->shortestPath(s, s_cave[0]);
        Graph *graph2 = new Graph(s_cave);
        std::vector<Point> sp2 = graph2->shortestPath(s, s_cave[s_cave.size() - 1]);

        delete (graph1);
        delete (graph2);

        std::vector<Point> sp = bypassHole(calculatePathLength(sp1),
                                      calculatePathLength(sp2), G::distance(d, p[d1]),
                                      G::distance(d, p[d2]), getVertexIndexInPolygon(s_cave[0], p),
                                      getVertexIndexInPolygon(s_cave[s_cave.size() - 1], p), d1, d2,
                                      p);

        // scale cave path
        if (sp1.back() == sp.front()) {
            if (sp1.size() == 2) {
                path.push_back(sp.front());
            } else {
                I = findHomotheticCenter(sp1, scaleFactor, k);
                for (unsigned int i = 1; i < sp1.size(); i++) {
                    Point anchor = Point(k * sp1[i].x_ + (1 - k) * I.x_, k * sp1[i].y_ + (1 - k) * I.y_);
                    path.push_back(anchor);
                }
            }
        } else if (sp2.back() == sp.front()) {
            if (sp2.size() == 2) {
                path.push_back(sp.front());
            } else {
                I = findHomotheticCenter(sp2, scaleFactor, k);
                for (unsigned int i = 1; i < sp2.size(); i++) {
                    Point anchor = Point(k * sp2[i].x_ + (1 - k) * I.x_, k * sp2[i].y_ + (1 - k) * I.y_);
                    path.push_back(anchor);
                }
            }
        }

        // scale convex path
//        if (sp.size() == 2) {
//            path.push_back(sp.back());
//        } else {

        std::vector<Point> sp_;
        sp_.push_back(s);
        sp_.insert(sp_.end(), sp.begin(), sp.end());
        sp_.push_back(d);
        I = findHomotheticCenter(sp_, scaleFactor, k);
        for (unsigned int i = 1; i < sp.size() - 1; i++) {
            Point anchor = Point(k * sp[i].x_ + (1 - k) * I.x_, k * sp[i].y_ + (1 - k) * I.y_);
            path.push_back(anchor);
//            }
        }
        path.push_back(d);
        return path;
    }

// Case 4: both s and d is inside convex hull
    else if (G::isPointInsidePolygon(s, p) && G::isPointInsidePolygon(d, p)) {
        std::vector<Point> s_cave = determineCaveContainingNode(s);
        std::vector<Point> d_cave = determineCaveContainingNode(d);
        if (s_cave.empty() || d_cave.empty())
            return path;
        if (s_cave[0] == d_cave[0] && s_cave[s_cave.size() - 1] == d_cave[d_cave.size() - 1]) {
            Graph *graph = new Graph(d_cave);
            std::vector<Point> sp = graph->shortestPath(s, d);

            // scale
            std::vector<Point> sp_;
            sp_.push_back(s);
            sp_.insert(sp_.end(), sp.begin(), sp.end());
            sp_.push_back(d);
            I = findHomotheticCenter(sp_, scaleFactor, k);
            for (auto it : sp) {
                Point anchor = Point(k * it.x_ + (1-k) * I.x_, k * it.y_ + (1-k) * I.y_);
                path.push_back(anchor);
            }
            path.push_back(d);
            return path;
        } else {
            Graph *dgraph1 = new Graph(d_cave);
            Graph *dgraph2 = new Graph(d_cave);
            Graph *sgraph1 = new Graph(s_cave);
            Graph *sgraph2 = new Graph(s_cave);

            std::vector<Point> s_sp1 = sgraph1->shortestPath(s, s_cave[0]);
            std::vector<Point> s_sp2 = sgraph2->shortestPath(s,
                                                        s_cave[s_cave.size() - 1]);

            std::vector<Point> d_sp1 = dgraph1->shortestPath(d,
                                                        d_cave[d_cave.size() - 1]);
            std::vector<Point> d_sp2 = dgraph2->shortestPath(d, d_cave[0]);

            delete (dgraph1);
            delete (dgraph2);
            delete (sgraph1);
            delete (sgraph2);

            std::vector<Point> sp = bypassHole(calculatePathLength(s_sp1),
                                          calculatePathLength(s_sp2), calculatePathLength(d_sp1),
                                          calculatePathLength(d_sp2),
                                          getVertexIndexInPolygon(s_cave[0], p),
                                          getVertexIndexInPolygon(s_cave[s_cave.size() - 1], p),
                                          getVertexIndexInPolygon(d_cave[d_cave.size() - 1], p),
                                          getVertexIndexInPolygon(d_cave[0], p), p);

            // scale s_cave path
            if (s_sp1.back() == sp.front()) {
                if (s_sp1.size() == 2) {
                    path.push_back(s_sp1.back());
                } else {
                    I = findHomotheticCenter(s_sp1, scaleFactor, k);
                    for (unsigned int i = 1; i < s_sp1.size(); i++) {
                        Point anchor = Point(k * s_sp1[i].x_ + (1 - k) * I.x_, k * s_sp1[i].y_ + (1 - k) * I.y_);
                        path.push_back(anchor);
                    }
                }
            } else if (s_sp2.back() == sp.front()) {
                if (s_sp2.size() == 2) {
                    path.push_back(s_sp2.back());
                } else {
                    I = findHomotheticCenter(s_sp2, scaleFactor, k);
                    for (unsigned int i = 1; i < s_sp2.size(); i++) {
                        Point anchor = Point(k * s_sp2[i].x_ + (1 - k) * I.x_, k * s_sp2[i].y_ + (1 - k) * I.y_);
                        path.push_back(anchor);
                    }
                }
            }

            // scale sp
//            if (sp.size() != 2) {
            std::vector<Point> sp_;
            sp_.push_back(s);
            sp_.insert(sp_.end(), sp.begin(), sp.end());
            sp_.push_back(d);
            I = findHomotheticCenter(sp, scaleFactor, k);
            for (unsigned int i = 1; i < sp.size() - 1; i++) {
                Point anchor = Point(k * sp[i].x_ + (1 - k) * I.x_, k * sp[i].y_ + (1 - k) * I.y_);
                path.push_back(anchor);
//                }
            }

            // scale d_cave path
            if (d_sp1.front() == sp.back()) {
                if (d_sp1.size() == 2) {
                    path.push_back(d_sp1.front());
                } else {
                    I = findHomotheticCenter(d_sp1, scaleFactor, k);
                    for (unsigned int i = 0; i < d_sp1.size() - 1; i++) {
                        Point anchor = Point(k * d_sp1[i].x_ + (1 - k) * I.x_, k * d_sp1[i].y_ + (1 - k) * I.y_);
                        path.push_back(anchor);
                    }
                }
            } else if (d_sp2.front() == sp.back()) {
                if (d_sp2.size() == 2) {
                    path.push_back(d_sp2.front());
                } else {
                    I = findHomotheticCenter(d_sp2, scaleFactor, k);
                    for (unsigned int i = 0; i < d_sp2.size() - 1; i++) {
                        Point anchor = Point(k * d_sp2[i].x_ + (1 - k) * I.x_, k * d_sp2[i].y_ + (1 - k) * I.y_);
                        path.push_back(anchor);
                    }
                }
            }
            path.push_back(d);
            return path;
        }
    }
}

Point VHRAgent::findHomotheticCenter(std::vector<Point> v, double lamda, double &k) {
    Point I = Point(0, 0);
    double fr = 0;
    for (auto it : v) {
//        srand(time(NULL));
        int ra = randSend_.uniform(1,10^6);
        I.x_ += it.x_ * ra;
        I.y_ += it.y_ * ra;
        fr += ra;
    }
    I.x_ = I.x_ / fr;
    I.y_ = I.y_ / fr;

    // calculate scale ratio
    double dis = G::distance(I, v[0]);
    double l = calculatePathLength(v);
    dis += l;
    dis += G::distance(I, v[v.size() - 1]);
    k = 1 + lamda * l / dis;

    FILE *f = fopen("Homothetic.tr", "a+");
    fprintf(f, "%f\t%f\t%f\t%f\t%f\t%f\t%f\n", v[0].x_, v[0].y_, v[v.size() - 1].x_, v[v.size() - 1].y_,
            I.x_, I.y_, k);
    fclose(f);
    return I;
}

int VHRAgent::getPathArraySize(Point *pth) {
    int i = 0;
    while (pth[i].x_ != NAN && pth[i].y_ != NAN)
        i++;
    return i;
}
int VHRAgent::getVertexIndexInPolygon(Point t, std::vector<Point> p) {
    for (unsigned int i = 0; i < p.size(); i++)
        if (p[i] == t)
            return i;
    return NAN;
}

double VHRAgent::calculatePathLength(std::vector<Point> p) {
    double length = 0;
    for (unsigned int i = 0; i < p.size() - 1; i++) {
        length += G::distance(p[i], p[i+1]);
    }
    return length;
}

// sDist1 = distance(S, p[s1]), sDist2 = distance(S, p[s2]); dDist1 = distance(D, p[d1]) dDist2 = distance(D, p[d2])
std::vector<Point> VHRAgent::bypassHole(double sDist1, double sDist2, double dDist1, double dDist2, int s1, int s2, int d1, int d2, std::vector<Point> p) {
    Point ms = G::midpoint(p[s1], p[s2]);
    Point md = G::midpoint(p[d1], p[d2]);

    if (G::position(p[s1], G::line(ms, md)) > 0) {
        int tmp = s1;
        s1 = s2;
        s2 = tmp;
        double tmp_ = sDist1;
        sDist1 = sDist2;
        sDist2 = tmp_;
    }
    if (G::position(p[d1], G::line(ms, md)) > 0) {
        int tmp = d1;
        d1 = d2;
        d2 = tmp;
        double tmp_ = dDist1;
        dDist1 = dDist2;
        dDist2 = tmp_;
    }
    double t1, t2, t3, t4, min;
    std::vector<Point> sp1, sp2, sp3, sp4, sp;
    //============================================= S S1 D1 D
    t1 = sDist1;
    if (d1 < s1) d1 += p.size();
    for (int i = s1; i < d1; i++) {
        t1 += G::distance(p[i % p.size()], p[(i+1) % p.size()]);
        sp1.push_back(p[i % p.size()]);
    }
    t1 += dDist1;
    sp1.push_back(p[d1 % p.size()]);
    min = t1; sp = sp1;

    //============================================= S S2 D2 D
    t2 = sDist2;
    if (d2 < s2) d2 += p.size();
    for (int i = s2; i < d2; i++) {
        t2 += G::distance(p[i % p.size()], p[(i+1) % p.size()]);
        sp2.push_back(p[i % p.size()]);
    }
    t2 += dDist2;
    sp2.push_back(p[d2 % p.size()]);
    if (t2 < min) { min = t2; sp = sp2;}

    //============================================= D D1 S1 S
    t3 = dDist1;
    if (s1 < d1) s1 += p.size();
    for (int i = d1; i < s1; i++) {
        t3 += G::distance(p[i % p.size()], p[(i+1) % p.size()]);
        sp3.push_back(p[i % p.size()]);
    }
    t3 += sDist1;
    sp3.push_back(p[s1 % p.size()]);
    reverse(sp3.begin(), sp3.end());
    if (t3 < min) { min = t3; sp = sp3;}

    //============================================= D D2 S2 S
    t4 = dDist2;
    if (s2 < d2) s2 += p.size();
    for (int i = d2; i < s2; i++) {
        t4 += G::distance(p[i % p.size()], p[(i+1) % p.size()]);
        sp4.push_back(p[i % p.size()]);
    }
    t4 += sDist2;
    sp4.push_back(p[s2 % p.size()]);
    reverse(sp4.begin(), sp4.end());
    if (t4 < min) {sp = sp4;}

    return sp;

}
// determine cave of hole that contains node s, cave is CCW order after this phase
std::vector<Point> VHRAgent::determineCaveContainingNode(Point s) {
    std::vector<BoundaryNode> convex = determineConvexHull();

    // rotate hole list, i.e. the first element is convex hull boundary
    int j = 0;
    while (!hole[j].is_convex_hull_boundary_)
        j++;
    rotate(hole.begin(), hole.begin() + j, hole.end());

    // determine cave containing s
    std::vector<Point> cave;
    for (unsigned int i = 0; i < hole.size() - 1; i++) {
        if (hole.at(i).is_convex_hull_boundary_ && s.x_ == hole.at(i).x_ && s.y_ == hole.at(i).y_) {
            break; // s is gate of cave & lies on convex hull's boundary
        }
        if (hole.at(i).is_convex_hull_boundary_ && !hole.at(i+1).is_convex_hull_boundary_) {
            cave.push_back(hole.at(i++));
            while (!hole.at(i).is_convex_hull_boundary_) {
                cave.push_back(hole.at(i++));
                if (i == hole.size() - 1)
                    break;
            }
            cave.push_back(hole.at(i--));
            if (i == hole.size() - 2)
                cave.push_back(hole.at(0));
            if (cave.size() >= MIN_CAVE_VERTICES) {
                if (G::isPointInsidePolygon(s, cave))
                    break;
            }
//            vector<Point>().swap(cave);
            cave.clear();
        }
    }
    return cave;
}