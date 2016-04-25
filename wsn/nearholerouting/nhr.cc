#include <packet.h>
#include <stack>
#include "nhr_packet.h"
#include "nhr_packet_data.h"
#include "nhr_graph.h"
#include <queue>

int hdr_nhr::offset_;

static class NHRHeaderClass : public PacketHeaderClass {
public:
    NHRHeaderClass() : PacketHeaderClass("PacketHeader/NHR", sizeof(hdr_nhr)) {
        bind_offset(&hdr_nhr::offset_);
    }
} class_nhrhdr;

static class NHRAgentClass : public TclClass {
public:
    NHRAgentClass() : TclClass("Agent/NHR") { }

    TclObject *create(int, const char *const *) {
        return (new NHRAgent());
    }
} class_nhr;

/*------------- Agent -------------*/
NHRAgent::NHRAgent() : BoundHoleAgent(), broadcast_timer_(this) {
    // initialize default value
    endpoint_.x_ = this->x_;
    endpoint_.y_ = this->y_; // default: endpoint of node is itself
    gate1_ = gate2_ = -1;
    isPivot_ = true;
    delta_ = 2;

    bind("delta_", &delta_);

    FILE *fp;
    fp = fopen("Neighbors.tr", "w");
    fclose(fp);
    fp = fopen("BoundHole.tr", "w");
    fclose(fp);
    fp = fopen("BroadcastRegion.tr", "w");
    fclose(fp);
    fp = fopen("Dump.tr", "w");
    fclose(fp);
    fp = fopen("Voronoi.tr", "w");
    fclose(fp);
    fp = fopen("ScalePolygon.tr", "w");
    fclose(fp);
}

void NHRAgent::recv(Packet *p, Handler *h) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_ip *iph = HDR_IP(p);

    switch (cmh->ptype()) {
        case PT_NHR:
            recvHCI(p);
            break;
        case PT_CBR:
            if (iph->saddr() == my_id_)                // a packet generated by myself
            {
                if (cmh->num_forwards() == 0)        // a new packet
                {
                    sendData(p);
                }
                else    //(cmh->num_forwards() > 0)	// routing loop -> drop
                {
                    drop(p, DROP_RTR_ROUTE_LOOP);
                    return;
                }
            }

            if (iph->ttl_-- <= 0) {
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

int NHRAgent::command(int argc, const char *const *argv) {
    return BoundHoleAgent::command(argc, argv);
}

/*------------- Hole approximation -------------*/
void NHRAgent::createHole(Packet *p) {
    int i = 0;
    vector<BoundaryNode> convex;

    polygonHole *h = createPolygonHole(p);
    for (struct node *node_tmp = h->node_list_; node_tmp; node_tmp = node_tmp->next_) {
        BoundaryNode bn;
        bn.x_ = node_tmp->x_;
        bn.y_ = node_tmp->y_;
        bn.is_convex_hull_boundary_ = false;
        bn.id_ = node_tmp->id_;
        hole_.push_back(bn);
    }
    delete h;

    // 1. Determine convex hull of hole using Graham scan algorithm
    convex = determineConvexHull();

    // 2. Approximate hole
    approximateHole(convex);

    // 3. Determine caves - rotate hole list, i.e. the first element is convex hull boundary
    i = 0;
    while (!hole_[i].is_convex_hull_boundary_) i++;
    rotate(hole_.begin(), hole_.begin() + i, hole_.end());
    hole_.push_back(hole_[0]); // circulate the hole list

    dump();
}

vector<BoundaryNode> NHRAgent::determineConvexHull() {
    stack<BoundaryNode *> hull;
    vector<BoundaryNode> convex;
    vector<BoundaryNode *> clone_hole;

    for (std::vector<BoundaryNode>::iterator it = hole_.begin(); it != hole_.end(); ++it) {
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

void NHRAgent::approximateHole(vector<BoundaryNode> convex) {
    /*
	 * define 8 lines for new approximate hole
	 *
	 * 		    ____[2]_________
	 * 		[02]	 			[21]______
	 * 	   /		 					  \
	 * 	[0] ----------------------------- [1]
	 * 	   \____                       ___/
	 * 			[30]____   ________[13]
	 * 					[3]
	 */
    int i, j;
    int n0, n1, n2, n3; // index of node h(p), h(q), h(k), h(j)
    Line bl;            // base line
    Line l0;            // line contain n0 and perpendicular with base line
    Line l1;            // line contain n1 and perpendicular with base line
    Line l2;            // line contain n2 and parallel with base line
    Line l3;            // line contain n3 and parallel with base line
    double longest_distance = 0;

    n0 = n1 = n2 = n3 = 0;

    // find couple node that have maximum distance - n0, n1
    for (i = 0; i < convex.size(); i++) {
        for (j = i; j < convex.size(); j++) {
            double tmp_distance = G::distance(convex[i], convex[j]);
            if (longest_distance < tmp_distance) {
                longest_distance = tmp_distance;
                n0 = i;
                n1 = j;
            }
        }
    }

    bl = G::line(convex[n0], convex[n1]); // bl = H(p)H(q)
    l0 = G::perpendicular_line(convex[n0], bl);
    l1 = G::perpendicular_line(convex[n1], bl);

    // find n2 and n3 - with maximum distance from base line
    longest_distance = 0;
    for (i = 0; i < convex.size(); i++) {
        double tmp_distance = G::distance(convex[i], bl);
        if (tmp_distance > longest_distance) {
            longest_distance = tmp_distance;
            n2 = i;
        }
    }
    longest_distance = 0;
    for (i = 0; i < convex.size(); i++) {
        double tmp_distance = G::distance(convex[i], bl);
        if (tmp_distance > longest_distance && G::position(convex[n2], bl) * G::position(convex[i], bl) < 0) {
            longest_distance = tmp_distance;
            n3 = i;
        }
    }

    l2 = G::parallel_line(convex[n2], bl);
    l3 = G::parallel_line(convex[n3], bl);

    // approximate hole
    BoundaryNode itsp; // intersect point (tmp)
    Line itsl;  // intersect line (tmp)
    double mc;

    // l02 intersection l0 and l2
    G::intersection(l0, l2, itsp);
    itsl = G::angle_bisector(convex[n0], convex[n1], itsp);
    longest_distance = 0;
    mc = 0;
    for (i = 0; i < convex.size(); i++) {
        itsl.c_ = -(itsl.a_ * convex[i].x_ + itsl.b_ * convex[i].y_);
        double tmp_distance = G::distance(convex[n3], itsl);
        if (tmp_distance > longest_distance) {
            longest_distance = tmp_distance;
            mc = itsl.c_;
        }
    }
    itsl.c_ = mc;
    G::intersection(l0, itsl, itsp);
    octagon_hole_.push_back(itsp);
    G::intersection(l2, itsl, itsp);
    octagon_hole_.push_back(itsp);

    // l21 intersection l2 and l1
    G::intersection(l1, l2, itsp);
    itsl = G::angle_bisector(convex[n1], convex[n0], itsp);
    longest_distance = 0;
    mc = 0;
    for (i = 0; i < convex.size(); i++) {
        itsl.c_ = -(itsl.a_ * convex[i].x_ + itsl.b_ * convex[i].y_);
        double tmp_distance = G::distance(convex[n3], itsl);
        if (tmp_distance > longest_distance) {
            longest_distance = tmp_distance;
            mc = itsl.c_;
        }
    }
    itsl.c_ = mc;
    G::intersection(l2, itsl, itsp);
    octagon_hole_.push_back(itsp);
    G::intersection(l1, itsl, itsp);
    octagon_hole_.push_back(itsp);

    // l13 intersection l1 and l3
    G::intersection(l1, l3, itsp);
    itsl = G::angle_bisector(convex[n1], convex[n0], itsp);
    longest_distance = 0;
    mc = 0;
    for (i = 0; i < convex.size(); i++) {
        itsl.c_ = -(itsl.a_ * convex[i].x_ + itsl.b_ * convex[i].y_);
        double dis = G::distance(convex[n2], itsl);
        if (dis > longest_distance) {
            longest_distance = dis;
            mc = itsl.c_;
        }
    }
    itsl.c_ = mc;
    G::intersection(l1, itsl, itsp);
    octagon_hole_.push_back(itsp);
    G::intersection(l3, itsl, itsp);
    octagon_hole_.push_back(itsp);

    // l30 intersection l3 and l0
    G::intersection(l0, l3, itsp);
    itsl = G::angle_bisector(convex[n0], convex[n1], itsp);
    longest_distance = 0;
    mc = 0;
    for (i = 0; i < convex.size(); i++) {
        itsl.c_ = -(itsl.a_ * convex[i].x_ + itsl.b_ * convex[i].y_);
        double dis = G::distance(convex[n2], itsl);
        if (dis > longest_distance) {
            longest_distance = dis;
            mc = itsl.c_;
        }
    }
    itsl.c_ = mc;
    G::intersection(l3, itsl, itsp);
    octagon_hole_.push_back(itsp);
    G::intersection(l0, itsl, itsp);
    octagon_hole_.push_back(itsp);
}

/*------------- Broadcast phase -------------*/
void NHRAgent::broadcastHCI() {
    Packet *p = NULL;
    NHRPacketData *payload;
    hdr_cmn *cmh;
    hdr_ip *iph;
    hdr_nhr *hhr;

    if (hole_.empty())
        return;

    p = allocpkt();
    payload = new NHRPacketData();
    for (std::vector<BoundaryNode>::iterator it = octagon_hole_.begin(); it != octagon_hole_.end(); ++it) {
        payload->add(-1, (*it).x_, (*it).y_, false);
    }
    for (std::vector<BoundaryNode>::iterator it = hole_.begin(); it != hole_.end(); ++it) {
        payload->add((*it).id_, (*it).x_, (*it).y_, (*it).is_convex_hull_boundary_);
    }
    p->setdata(payload);

    cmh = HDR_CMN(p);
    iph = HDR_IP(p);
    hhr = HDR_NHR(p);

    cmh->ptype() = PT_NHR;
    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = IP_BROADCAST;
    cmh->last_hop_ = my_id_;
    cmh->addr_type_ = NS_AF_INET;
    cmh->size() += IP_HDR_LEN + hhr->size();

    iph->daddr() = IP_BROADCAST;
    iph->saddr() = my_id_;
    iph->sport() = RT_PORT;
    iph->dport() = RT_PORT;
    iph->ttl_ = 4 * IP_DEF_TTL;

    Point agent;
    agent.x_ = this->x_;
    agent.y_ = this->y_;
    NHRGraph *graph = new NHRGraph(agent, hole_);
    endpoint_ = graph->endpoint();
    isPivot_ = graph->isPivot();
    graph->getGateNodeIds(gate1_, gate2_);
    delete graph;

    send(p, 0);
}

void NHRAgent::recvHCI(Packet *p) {
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

    // check if is really receive this hole's information
    if (!hole_.empty())    // already received
    {
        drop(p, "HciReceived");
        return;
    }

    // store hci
    NHRPacketData *data = (NHRPacketData *) p->userdata();
    for (int i = 1; i <= 8; i++) {
        BoundaryNode n = data->get_data(i);
        octagon_hole_.push_back(n);
    }
    for (int i = 9; i <= data->size(); i++) {
        BoundaryNode n = data->get_data(i);
        hole_.push_back(n);
    }

    if (!canBroadcast()) {
        drop(p, "OutsideRegion");
    }
    else {
        broadcast_timer_.setParameter(p);
        broadcast_timer_.resched(randSend_.uniform(0.0, 1.0));
    }

    // construct graph
    Point agent;
    agent.x_ = this->x_;
    agent.y_ = this->y_;
    NHRGraph *graph = new NHRGraph(agent, hole_);
    endpoint_ = graph->endpoint();
    isPivot_ = graph->isPivot();
    graph->getGateNodeIds(gate1_, gate2_);
    delete graph;

    dumpBroadcastRegion();
}

// check if node is inside NHR region. also calculate the scale factor = min distance from node to octagon
bool NHRAgent::canBroadcast() {
    vector<BoundaryNode *> convex;

    // construct temporary node list
    polygonHole *node_list = new polygonHole();
    node_list->node_list_ = NULL;
    node_list->next_ = NULL;

    for (int i = 0; i < octagon_hole_.size(); i++) {
        convex.push_back(&octagon_hole_[i]);
        node *tmp = new node();
        tmp->x_ = octagon_hole_[i].x_;
        tmp->y_ = octagon_hole_[i].y_;
        tmp->next_ = node_list->node_list_;
        node_list->node_list_ = tmp;
    }

    double distance = distanceToPolygon(node_list->node_list_);
    delete node_list;
    if (distance <= delta_) {
        calculateScaleFactor(distance);
        return true;
    }

    return false;
}

void NHRAgent::calculateScaleFactor(double d) {
    delta_ = d;
}

double NHRAgent::distanceToPolygon(node *polygon) {
    node *tmp0;
    node *tmp1;
    node *tmp2;
    node *tmp3;
    Line l, l1, l2; // l: line P(i+1)P(i+2),l1: parallel with P(i)P(i+1), l2: parallel with P(i+2)P(i+3)

    // check if point inside polygon return maximum scale factor
    if (G::isPointInsidePolygon(this, polygon)) return delta_;

    double distance;
    double d = DBL_MAX;
    int n = 8; // number of vertices of octagon = 8

    for (tmp0 = polygon; n; tmp0 = tmp0->next_, n--) {
        tmp1 = tmp0->next_ == NULL ? polygon : tmp0->next_;
        tmp2 = tmp1->next_ == NULL ? polygon : tmp1->next_;
        l = G::line(tmp1, tmp2);

        if (G::position(this, tmp0, &l) > 0) continue;

        distance = G::distance(this, l);
        if (distance < d) {
            // detect if node stays on covering polygon's boundary
            tmp3 = tmp2->next_ == NULL ? polygon : tmp2->next_;
            l1 = G::line(tmp0, tmp1);
            l2 = G::line(tmp2, tmp3);

            int pos1 = G::position(this, tmp2, &l1);
            int pos2 = G::position(this, tmp1, &l2);

            if (pos1 * pos2 >= 0 && pos1 >= 0) {
                d = distance;
            } else if (pos1 * pos2 >= 0 && pos1 < 0) {
                if (G::distance(this, l1) <= distance && G::distance(this, l2) <= distance)
                    d = distance;
            } else {
                if ((pos1 < 0 && G::distance(this, l1) <= distance)
                    || (pos2 < 0 && G::distance(this, l2) <= distance)) {
                    d = distance;
                }
            }
        }
    }

    return d / range_;
}

/*------------- Sending Data -------------*/
void NHRAgent::sendData(Packet *p) {
    hdr_cmn *cmh = HDR_CMN(p);
    hdr_ip *iph = HDR_IP(p);
    hdr_nhr *edh = HDR_NHR(p);

    cmh->size() += IP_HDR_LEN + edh->size();
    cmh->direction_ = hdr_cmn::DOWN;

    edh->daddr_ = iph->daddr();
    edh->ap_index = 0;
    edh->anchor_points[1] = endpoint_;
    edh->type = NHR_CBR_GPSR;
    edh->dest_level = 0;
    edh->anchor_points[0] = *dest;
    edh->dest_ = *dest;

    iph->saddr() = my_id_;
    iph->daddr() = -1;
    iph->ttl_ = 4 * IP_DEF_TTL;
}

void NHRAgent::recvData(Packet *p) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_nhr *edh = HDR_NHR(p);
    node *nexthop = NULL;

    if (cmh->direction() == hdr_cmn::UP && edh->daddr_ == my_id_)    // up to destination
    {
        port_dmux_->recv(p, 0);
        return;
    }

    switch (edh->type) {
        case NHR_CBR_GPSR:
            if (!hole_.empty()) {
                edh->type = NHR_CBR_AWARE_SOURCE_ESCAPE;
                edh->ap_index = 1;
            }
            break;
        case NHR_CBR_AWARE_SOURCE_ESCAPE:
            edh->anchor_points[1] = endpoint_;
            if (isPivot_) {
                edh->type = NHR_CBR_AWARE_SOURCE_PIVOT;
            }
            break;
        default:
            break;
    }

    nexthop = getNeighborByGreedy(edh->anchor_points[edh->ap_index]);
    if (nexthop == NULL) {
        switch (edh->type) {
            case NHR_CBR_AWARE_SOURCE_ESCAPE:
            case NHR_CBR_AWARE_SOURCE_PIVOT:
                if (determineOctagonAnchorPoints(p)) {
                    edh->type = NHR_CBR_AWARE_OCTAGON;
                } else {
                    edh->ap_index = 0;
                    edh->type = NHR_CBR_AWARE_DESTINATION;
                }
                break;
            case NHR_CBR_AWARE_OCTAGON:
                if (edh->ap_index == 2) {
                    edh->ap_index = 0;
                    edh->type = NHR_CBR_AWARE_DESTINATION;
                } else {
                    --edh->ap_index;
                }
                break;
            case NHR_CBR_AWARE_DESTINATION:
                routeToDest(p);
                break;
            default:
                drop(p, DROP_RTR_NO_ROUTE);
                return;
        }
    }

    nexthop = getNeighborByGreedy(edh->anchor_points[edh->ap_index]);
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

Point NHRAgent::calculateDestEndpoint(Point dest, int &level, int &gate1, int &gate2) {
    // TODO: if dest, source in the same cave
    // return to routeToDest mode immediately
    Point gate_point;
    NHRGraph *graph = new NHRGraph(dest, hole_);
    gate_point = graph->isPivot() ? graph->endpoint() : graph->gatePoint(level);
    graph->getGateNodeIds(gate1, gate2);
    delete graph;
    return gate_point;
}

// return false if cannot determine octagon
bool NHRAgent::determineOctagonAnchorPoints(Packet *p) {
    struct hdr_nhr *hdc = HDR_NHR(p);

    // calculate dest endpoint
    int d_gate1, d_gate2;
    d_gate1 = d_gate2 = -1;
    hdc->anchor_points[0] = calculateDestEndpoint(hdc->dest_, hdc->dest_level, d_gate1, d_gate2);

    // if sd doesnt intersect with the hole -> go by GPSR (set hdc->ap_index = 0)
    if (!sdPolygonIntersect(p)) {
        return false;
    }
    // else calculate the scale octagon

    // random I
    double scale_factor;
    Point I;
    I.x_ = 0;
    I.y_ = 0;
    double fr = 0;
    for (int i = 0; i < octagon_hole_.size(); i++) {
        int ra = randSend_.uniform_positive_int();
        I.x_ += octagon_hole_[i].x_ * ra;
        I.y_ += octagon_hole_[i].y_ * ra;
        fr += ra;
    }

    I.x_ = I.x_ / fr;
    I.y_ = I.y_ / fr;

    double dis = G::distance(I, octagon_hole_[0]);
    for (int i = 1; i < octagon_hole_.size(); i++) {
        double tmp = G::distance(I, octagon_hole_[i]);
        if (tmp > dis) {
            dis = tmp;
        }
    }

    scale_factor = (dis + delta_ * range_) / dis;

    // scale hole by I and scale_factor
    vector<BoundaryNode> scaleHole;

    for (int i = 0; i < octagon_hole_.size(); i++) {
        BoundaryNode newPoint;
        newPoint.x_ = scale_factor * octagon_hole_[i].x_ + (1 - scale_factor) * I.x_;
        newPoint.y_ = scale_factor * octagon_hole_[i].y_ + (1 - scale_factor) * I.y_;
        scaleHole.push_back(newPoint);
    }
    dumpScalePolygon(scaleHole, I);
    // generate new anchor points
    struct SourceDest sourceDest;
    sourceDest.source = *this;
    sourceDest.dest = hdc->anchor_points[0];
    sourceDest.s_gate1 = gate1_;
    sourceDest.s_gate2 = gate2_;
    sourceDest.d_gate1 = d_gate1;
    sourceDest.d_gate2 = d_gate2;
    bypassHole(p, sourceDest, scaleHole);
    return true;
}

// source - dest-endpoint intersects with hole or not
bool NHRAgent::sdPolygonIntersect(Packet *p) {
    struct hdr_nhr *hdc = HDR_NHR(p);
    Point dest = hdc->anchor_points[0];
    int num_intersection = 0;

    for (int i = 0; i < hole_.size() - 1; i++) {
        int j = (i == hole_.size() - 1 ? 0 : (i + 1));
        if (G::is_in_line(this, &dest, hole_[i]) && G::is_in_line(this, &dest, hole_[j])) break;
        if (G::is_intersect(this, &dest, hole_[i], hole_[j])) num_intersection++;
    }

    return num_intersection > 0;
}

bool NHRAgent::isPointInsidePolygon(Point p, vector<BoundaryNode> polygon) {
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

void NHRAgent::bypassHole(Packet *p, struct SourceDest sourceDest, vector<BoundaryNode> scaleOctagon) {
    struct hdr_nhr *edh = HDR_NHR(p);

    int si1, si2, di1, di2;
    Line sd = G::line(sourceDest.source, sourceDest.dest);
    vector<Point> aps;

    vector<BoundaryNode> convex;
    for (int i = 0; i < hole_.size(); i++) {
        if (hole_[i].is_convex_hull_boundary_) {
            convex.push_back(hole_[i]);
        }
    }

    findLimitAnchorPoint(&sourceDest.source, sourceDest.s_gate1, sourceDest.s_gate2, scaleOctagon, convex, si1, si2);
    findLimitAnchorPoint(&sourceDest.dest, sourceDest.d_gate1, sourceDest.d_gate2, scaleOctagon, convex, di1, di2);

    if (G::position(scaleOctagon[si1], sd) * G::position(scaleOctagon[di1], sd) < 0) {
        int tmp = si2;
        si2 = si1;
        si1 = tmp;
    }

    // octagon order: counter clockwise
    // si -> di: counter clockwise
    // si2 -> di2: clockwise
    if (di1 < si1) di1 += 8;
    if (si2 < di2) si2 += 8;

    double length = 0;
    double dis = 0;

    // double scaleOctagon
    for (int i = 0; i < 8; ++i) {
        scaleOctagon.push_back(scaleOctagon[i]);
    }

    // s - si1 - di1 - d
    dis = G::distance(sourceDest.source, scaleOctagon[si1]);
    for (int i = si1; i < di1; i++) {
        dis += G::distance(scaleOctagon[i], scaleOctagon[i + 1]);
    }
    dis += G::distance(scaleOctagon[di1], sourceDest.dest);
    length = dis;
    for (int i = di1; i >= si1; --i) {
        aps.push_back(scaleOctagon[i]);
    }

    // s - si2 - di2 - d
    dis = G::distance(sourceDest.source, scaleOctagon[si2]);
    for (int i = di2; i < si2; ++i) {
        dis += G::distance(scaleOctagon[i], scaleOctagon[i + 1]);
    }
    dis += G::distance(scaleOctagon[di2], sourceDest.dest);
    if (dis < length) {
        vector<Point>().swap(aps);
        for (int i = di2; i <= si2; ++i) {
            aps.push_back(scaleOctagon[i]);
        }
    }

    // update routing table
    for (vector<Point>::iterator it = aps.begin(); it != aps.end(); ++it) {
        edh->anchor_points[++edh->ap_index] = *it;
    }
}

void NHRAgent::findLimitAnchorPoint(Point *point, int gate1, int gate2,
                                    vector<BoundaryNode> scaleHole, vector<BoundaryNode> hole,
                                    int &i1, int &i2) {
    if (!isPointInsidePolygon(*point, scaleHole)) {
        // point is outside scaleHole
        findViewLimitVertices(*point, scaleHole, i1, i2);
    } else {
        int it1, it2;
        it1 = it2 = 0;
        if (gate1 != -1 && gate2 != -1) { // work around for floating point round error
            for (int i = 0; i < hole.size(); i++) {
                if (hole[i].id_ == gate1) {
                    it1 = i;
                } else if (hole[i].id_ == gate2) {
                    it2 = i;
                }
            }
        } else {
            // point lies between scaleHole & hole
            findViewLimitVertices(*point, hole, it1, it2);
        }
        Line l1 = G::line(point, hole.at((uint) it1));
        Line l2 = G::line(point, hole.at((uint) it2));
        for (int i = 0; i < 8; i++) {
            Point ins;
            if (G::lineSegmentIntersection(&scaleHole[i], &scaleHole[(i + 1) % 8], l1, ins) &&
                G::onSegment(*point, hole.at((uint) it1), ins)) {
                if (G::orientation(*point, scaleHole[i], ins) == 1) {
                    i1 = i;
                } else {
                    i1 = (i + 1) % 8;
                }
            } else if (G::lineSegmentIntersection(&scaleHole[i], &scaleHole[(i + 1) % 8], l2, ins) &&
                       G::onSegment(*point, hole.at((uint) it2), ins)) {
                if (G::orientation(*point, scaleHole[i], ins) == 2) {
                    i2 = i;
                } else {
                    i2 = (i + 1) % 8;
                }
            }
        }
    }
}

void NHRAgent::findViewLimitVertices(Point point, vector<BoundaryNode> polygon, int &i1, int &i2) {
    vector<BoundaryNode *> clone;
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

void NHRAgent::routeToDest(Packet *p) {
    struct hdr_nhr *edh = HDR_NHR(p);
    NHRGraph *graph = new NHRGraph(edh->dest_, hole_);
    node *nexthop = getNeighborByGreedy(edh->anchor_points[0]);
    while (nexthop == NULL || edh->anchor_points[0] != edh->dest_) {
        edh->anchor_points[0] = graph->traceBack(edh->dest_level);
        if (edh->dest_level == 0) break;
        nexthop = getNeighborByGreedy(edh->anchor_points[0]);
    }
    delete graph;
}

/*------------- Dump -------------*/
void NHRAgent::dump() {
    FILE *fp = fopen("Dump.tr", "a+");
    for (std::vector<BoundaryNode>::iterator it = hole_.begin(); it != hole_.end(); ++it)
        fprintf(fp, "%d\t%f\t%f\t%d\n", (*it).id_, (*it).x_, (*it).y_, (*it).is_convex_hull_boundary_);
    for (std::vector<BoundaryNode>::iterator it = octagon_hole_.begin(); it != octagon_hole_.end(); ++it) {
        fprintf(fp, "%f\t%f\n", (*it).x_, (*it).y_);
    }
    fclose(fp);
}

void NHRAgent::dumpBroadcastRegion() {
    FILE *fp = fopen("BroadcastRegion.tr", "a+");
    fprintf(fp, "%d\t%f\t%f\n", my_id_, x_, y_);
    fclose(fp);
}

void NHRAgent::dumpScalePolygon(vector<BoundaryNode> scale, Point center) {
    FILE *fp = fopen("ScalePolygon.tr", "a+");
    fprintf(fp, "%f\t%f\n\n", center.x_, center.y_);
    for (int i = 0; i < scale.size(); ++i) {
        fprintf(fp, "%f\t%f\n", scale[i].x_, scale[i].y_);
    }
    fprintf(fp, "\n");
    fclose(fp);
}
