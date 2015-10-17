//
// Created by eleven on 10/7/15.
//

#include "bcpcoverage.h"
#include "../include/tcl.h"
#include "bcpcoverage_packet_data.h"

#define EPSILON 0.0001


int hdr_bcpcoverage::offset_;

/*
 * Coverage Header Class
 */
static class BCPCoverageHeaderClass : public PacketHeaderClass {
public:
    BCPCoverageHeaderClass() : PacketHeaderClass("PacketHeader/BCPCOVERAGE", sizeof(hdr_bcpcoverage)) {
        bind_offset(&hdr_bcpcoverage::offset_);
    }

    ~BCPCoverageHeaderClass() { }
} class_coveragehdr;

/*
 * Coverage Agent Class
 */
static class BCPCoverageAgentClass : public TclClass {
public:
    BCPCoverageAgentClass() : TclClass("Agent/BCPCOVERAGE") { }

    TclObject *create(int, const char *const *) {
        return (new BCPCoverageAgent());
    }
} class_bcpcoverage;

void
BCPCoverageTimer::expire(Event *e) {
    ((BCPCoverageAgent *) a_->*firing_)();
}

// ------------------------ Agent ------------------------ //
BCPCoverageAgent::BCPCoverageAgent() : GPSRAgent(), boundhole_timer_(this, &BCPCoverageAgent::holeBoundaryDetection) {
    hole_list_ = NULL;
    bcp_list = NULL;

    bind("range_", &communication_range_);
    bind("limit_boundhole_hop_", &limit_hop);
    bind("degree_coverage_", &degree_coverage_);
    sensor_range_ = 0.5 * communication_range_;
}

int BCPCoverageAgent::command(int argc, const char *const *argv) {
    if (strcasecmp(argv[1], "start") == 0) {
        startUp();
    } else if (strcasecmp(argv[1], "dump") == 0) {
        dumpSensorNeighbor();
    } else if (strcasecmp(argv[1], "coverage") == 0) {
        bcpDetection();
        boundhole_timer_.resched(10 + randSend_.uniform(0.0, 5));
        return TCL_OK;
    }

    return GPSRAgent::command(argc, argv);
}

void BCPCoverageAgent::recv(Packet *p, Handler *h) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_ip *iph = HDR_IP(p);

    switch (cmh->ptype()) {
        case PT_BCPCOVERAGE:
            recvCoverage(p);
            break;
        default:
            GPSRAgent::recv(p, h);
    }
}

void BCPCoverageAgent::recvCoverage(Packet *p) {
    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_cmn *cmh = HDR_CMN(p);

    BCPCoveragePacketData *data = (BCPCoveragePacketData *) p->userdata();

    if (data->size() == 0) {
        drop(p);
        return;
    }

    // if the boundhole packet has came back to the initial node
    node firstNode = data->get_data(0);
    node lastNode = data->get_data(data->size() - 1);
    lastNode.id_ = cmh->last_hop_;
    node *nextBCP = getNextBCP(&lastNode);
    if (data->size() >= 3 && firstNode.id_ == nextBCP->id_) {
        node *head = NULL;
        for (int i = 0; i < data->size(); i++) {
            node n = data->get_data(i);
            node *item = new node();
            item->x_ = n.x_;
            item->y_ = n.y_;
            item->id_ = n.id_;
            item->next_ = head;
            head = item;
        }
        head = reduceBCP(head);
        polygonHole *newHole = new polygonHole();
        newHole->node_list_ = head;
        newHole->next_ = hole_list_;
        hole_list_ = newHole;

        dumpCoverageBoundhole(newHole);
//        data->dump();
//      printf("%d Boundhole is detected, nodeNumberEstimation: %d\n", my_id_, nodeNumberEstimation(newHole));
        drop(p, "BCPCOVERAGE");
        return;
    }

    if (iph->ttl_-- <= 0) {
        drop(p, DROP_RTR_TTL);
        return;
    }

    if (iph->saddr() < my_id_) {
        drop(p, " REPEAT");
        return;
    }

    data->add(nextBCP->id_, nextBCP->x_, nextBCP->y_);

    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = nextBCP->id_;
    cmh->last_hop_ = my_id_;

    iph->daddr() = nextBCP->id_;

    send(p, 0);
}

/*----------------------------BOUNDARY DETECTION------------------------------------------------------*/
bool BCPCoverageAgent::checkBCP(node *pNode) {
    int count = 0;
    for (node *temp = neighbor_list_; temp; temp = temp->next_) {
        double d = G::distance(temp, pNode);
        if (fabs(d - sensor_range_) < EPSILON) continue;
        else if (d < sensor_range_) {
//            printf("Diff: %g\n", sensor_range_ - d);
            count++;
        }

        if (count >= degree_coverage_) return false;
    }

    return true;
}

node *BCPCoverageAgent::getBCP(Point point) {
    for (node *tmp = bcp_list; tmp; tmp = tmp->next_)
        if (fabs(tmp->x_ - point.x_) < EPSILON && fabs(tmp->y_ - point.y_) < EPSILON) return tmp;
    return NULL;
}

void BCPCoverageAgent::updateBCP(node *pNode) {
    Angle max = 0;
    int32_t addr = -1;
    for (node *tmp = neighbor_list_; tmp; tmp = tmp->next_) {
        Angle angle = G::angle(this, tmp, this, pNode);
        if (max < angle) {
            max = angle;
            addr = tmp->id_;
        }
    }
    pNode->id_ = addr;
}

void BCPCoverageAgent::bcpDetection() {
    for (sensor_neighbor *n = (sensor_neighbor *) neighbor_list_; n; n = (sensor_neighbor *) n->next_) {
        // add intersect 1
        node *temp = getBCP(n->i1_);
        if (temp == NULL) {
            temp = new node;
            temp->x_ = n->i1_.x_;
            temp->y_ = n->i1_.y_;
            temp->id_ = n->id_;
            temp->next_ = NULL;

            if (checkBCP(temp)) {
                if (bcp_list == NULL) {
                    bcp_list = temp;
                } else {
                    Angle angle = G::angle(this, bcp_list, this, temp);
                    node *i;
                    for (i = bcp_list; i->next_; i = i->next_) {
                        if (G::angle(this, bcp_list, this, i->next_) > angle) {
                            temp->next_ = i->next_;
                            i->next_ = temp;
                            break;
                        }
                    }

                    if (i->next_ == NULL)    // if angle is maximum, add temp to end of neighobrs list
                    {
                        i->next_ = temp;
                    }
                }
            }
        } else {
            updateBCP(temp);
        }

        // check intersect 2
        if (n->i2_.x_ == n->i1_.x_ && n->i2_.y_ == n->i1_.y_) continue;
        // if not same as intersect 1, add intersect 2
        temp = getBCP(n->i2_);
        if (temp == NULL) {
            temp = new node;
            temp->x_ = n->i2_.x_;
            temp->y_ = n->i2_.y_;
            temp->id_ = n->id_;
            temp->next_ = NULL;

            if (checkBCP(temp)) {
                if (bcp_list == NULL) {
                    bcp_list = temp;
                } else {
                    Angle angle = G::angle(this, bcp_list, this, temp);
                    node *i;
                    for (i = bcp_list; i->next_; i = i->next_) {
                        if (G::angle(this, bcp_list, this, i->next_) > angle) {
                            temp->next_ = i->next_;
                            i->next_ = temp;
                            break;
                        }
                    }

                    if (i->next_ == NULL)    // if angle is maximum, add temp to end of neighobrs list
                    {
                        i->next_ = temp;
                    }
                }
            }
        } else {
            updateBCP(temp);
        }
    }
}

node *BCPCoverageAgent::getNextBCP(node *pNode) {
    neighbor *n = getNeighbor(pNode->id_);
    if (G::angle(this, pNode, this, n) >= M_PI) {
        node *bcp = getBCP(*pNode);
        return bcp->next_ == NULL ? bcp_list : bcp->next_;
    }

    return NULL;
}

void BCPCoverageAgent::holeBoundaryDetection() {
    if (bcp_list == NULL) return;

    dumpBoundaryDetect();
    Packet *p;
    hdr_cmn *cmh;
    hdr_ip *iph;
    hdr_bcpcoverage *ch;

    for (node *n = bcp_list; n; n = n->next_) {
        node *nextBCP = getNextBCP(n);
        if (nextBCP != NULL) continue;

        p = allocpkt();

        BCPCoveragePacketData *chpkt_data = new BCPCoveragePacketData();
        chpkt_data->add(n->id_, n->x_, n->y_);
        p->setdata(chpkt_data);

        cmh = HDR_CMN(p);
        iph = HDR_IP(p);
        ch = HDR_BCPCOVERAGE(p);

        cmh->ptype() = PT_BCPCOVERAGE;
        cmh->direction() = hdr_cmn::DOWN;
        cmh->size() += IP_HDR_LEN + ch->size() + chpkt_data->data_len_;
        cmh->next_hop_ = n->id_;
        cmh->last_hop_ = my_id_;
        cmh->addr_type_ = NS_AF_INET;

        iph->saddr() = my_id_;
        iph->daddr() = n->id_;
        iph->sport() = RT_PORT;
        iph->dport() = RT_PORT;
        iph->ttl_ = limit_hop;            // more than ttl_ hop => boundary => remove

        send(p, 0);
    }
}

int BCPCoverageAgent::nodeNumberEstimation(polygonHole *pHole) {
    return 0;
}

/*----------------Utils function----------------------*/

void
BCPCoverageAgent::startUp() {
    FILE *fp;
    fp = fopen("SensorNeighbors.tr", "w");
    fclose(fp);
    fp = fopen("NodeBoundaryDetect.tr", "w");
    fclose(fp);
    fp = fopen("BCPCoverage.tr", "w");
    fclose(fp);
}

void BCPCoverageAgent::addNeighbor(nsaddr_t nid, Point location) {
    sensor_neighbor *temp = (sensor_neighbor *) getNeighbor(nid);

    if (temp == NULL)            // it is a new node
    {
        temp = new sensor_neighbor;
        temp->id_ = nid;
        temp->x_ = location.x_;
        temp->y_ = location.y_;
        temp->time_ = Scheduler::instance().clock();
        G::circleCircleIntersect(this, sensor_range_, temp, sensor_range_, &temp->i1_, &temp->i2_);
        temp->next_ = NULL;

        if (neighbor_list_ == NULL)        // the list now is empty
        {
            neighbor_list_ = temp;
        }
        else                        // the nodes list is not empty
        {
            Angle angle = G::angle(this, neighbor_list_, this, temp);
            node *i;
            for (i = neighbor_list_; i->next_; i = i->next_) {
                if (G::angle(this, neighbor_list_, this, i->next_) > angle) {
                    temp->next_ = i->next_;
                    i->next_ = temp;
                    break;
                }
            }

            if (i->next_ == NULL)    // if angle is maximum, add temp to end of neighobrs list
            {
                i->next_ = temp;
            }
        }
    }
    else // temp != null
    {
        temp->time_ = NOW;
        temp->x_ = location.x_;
        temp->y_ = location.y_;
    }
}

node *BCPCoverageAgent::reduceBCP(node *list) {
    node *temp, *nextAdjacent, *n, *prev, *temp2;
    double min = G::distance(list, list->next_);
    temp2 = list;
    bool check = true;
    Point c, n0_a_, n0_b_;
    // find the shortest edge
    for(temp = list->next_; temp; temp = temp->next_){
        n = temp->next_ == NULL ? list : temp->next_;
        double d = G::distance(n, temp);
        if (min > d){
            temp2 = temp;
            min = d;
        }
    }

    temp = temp2;

    n = temp->next_ == NULL ? list : temp->next_;
    nextAdjacent = n->next_ == NULL ? list : n->next_;
    prev = n;
    G::circleCircleIntersect(temp, sensor_range_, prev, sensor_range_, &c, &n0_b_);

    check = true;
    while (check && G::distance(temp, nextAdjacent) <= 2 * sensor_range_) {
        // detect N0
        G::circleCircleIntersect(temp, sensor_range_, nextAdjacent, sensor_range_, &n0_a_, &n0_b_);

        for (node *i = n; i != nextAdjacent; i = i->next_ == NULL ? list : i->next_) {
            if (G::distance(n0_a_, i) > sensor_range_) {
                check = false;
                break;
            }
        }

//            if (!check) {
//                check = true;
//                for (node *i = temp->next_; i != nextAdjacent; i = i->next_ == NULL ? list : i->next_) {
//                    if (G::distance(n0_b_, i) > sensor_range_) {
//                        check = false;
//                        break;
//                    }
//                }
//            } else {
//                c.x_ = n0_a_.x_;
//                c.y_ = n0_a_.y_;
//            }

        if (check){
            c.x_ = n0_b_.x_;
            c.y_ = n0_b_.y_;
        }

        if (nextAdjacent == temp) break;

        if (check) {
            prev = nextAdjacent;
            nextAdjacent = nextAdjacent->next_ == NULL ? list : nextAdjacent->next_;
        }
    }

    if (check){
        printf("New point: (%f,%f)\n", c.x_, c.y_);
    }

    return list;
}

/*-------------------------- DUMP --------------*/
void BCPCoverageAgent::dumpSensorNeighbor() {

}

void BCPCoverageAgent::dumpBoundaryDetect() {
    FILE *fp;
    fp = fopen("NodeBoundaryDetect.tr", "a");
    fprintf(fp, "%d\t%f\t%f", my_id_, x_, y_);
    for (node *n = bcp_list; n; n = n->next_) {
        fprintf(fp, "\t%d(%f,%f)", n->id_, n->x_, n->y_);
    }
    fprintf(fp, "\n");
    fclose(fp);
}

void BCPCoverageAgent::dumpCoverageBoundhole(polygonHole* hole) {
    FILE *fp;
    fp = fopen("BCPCoverage.tr", "a");
    for(node* temp = hole->node_list_; temp; temp= temp->next_){
        fprintf(fp, "%d\t%f\t%f\n", temp->id_, temp->x_, temp->y_);
    }
    fprintf(fp, "\n");
    fclose(fp);
}
