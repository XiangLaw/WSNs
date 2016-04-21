#include "hach.h"
#include "hach_packet_data.h"

int hdr_hach::offset_;

/*
 * Coverage Header Class
 */
static class HACHHeaderClass : public PacketHeaderClass {
public:
    HACHHeaderClass() : PacketHeaderClass("PacketHeader/HACH", sizeof(hdr_hach)) {
        bind_offset(&hdr_hach::offset_);
    }

    ~HACHHeaderClass() { }
} class_hachhdr;

/*
 * Coverage Agent Class
 */
static class HACHAgentClass : public TclClass {
public:
    HACHAgentClass() : TclClass("Agent/HACH") { }

    TclObject *create(int, const char *const *) {
        return (new HACHAgent());
    }
} class_hach;

void
HACHTimer::expire(Event *e) {
    ((HACHAgent *) a_->*firing_)();
}

// ------------------------ Agent ------------------------ //
HACHAgent::HACHAgent() : GPSRAgent(), boundhole_timer_(this, &HACHAgent::holeBoundaryDetection) {
    hole_list_ = NULL;
    bcp_list = NULL;

    bind("range_", &communication_range_);
    bind("limit_boundhole_hop_", &limit_hop);
    bind("degree_coverage_", &degree_coverage_);
    sensor_range_ = 0.5 * communication_range_;
}

int HACHAgent::command(int argc, const char *const *argv) {
    if (strcasecmp(argv[1], "start") == 0) {
        startUp();
    } else if (strcasecmp(argv[1], "coverage") == 0) {
        runTimeCounter.start();
        bcpDetection();
        // dumpBoundaryDetect();
        boundhole_timer_.resched(10 + 0.02 * my_id_);
        return TCL_OK;
    }

    return GPSRAgent::command(argc, argv);
}

void HACHAgent::recv(Packet *p, Handler *h) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_ip *iph = HDR_IP(p);

    switch (cmh->ptype()) {
        case PT_HACH:
            recvHACH(p);
            break;
        case PT_BCPCOVERAGE:
            recvCoverage(p);
            break;
        default:
            GPSRAgent::recv(p, h);
    }
}

void HACHAgent::recvCoverage(Packet *p) {
    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_cmn *cmh = HDR_CMN(p);

    HACHPacketData *data = (HACHPacketData *) p->userdata();

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
        for (int i = data->size() - 1; i >= 0; i--) {
            node n = data->get_data(i);
            node *item = new node();
            item->x_ = n.x_;
            item->y_ = n.y_;
            item->id_ = n.id_;
            item->next_ = head;
            head = item;
        }

        polygonHole *newHole = new polygonHole();
        newHole->node_list_ = head;
        newHole->next_ = hole_list_;
        hole_list_ = newHole;

        dumpCoverageBoundhole(newHole);
        drop(p, "BCPCOVERAGE");

        sendHACH(lastNode, *nextBCP);
        return;
    }

    if (iph->ttl_-- <= 0) {
        drop(p, DROP_RTR_TTL);
        return;
    }

    if (iph->saddr() < my_id_) {
        drop(p, "REPEAT");
        return;
    }

    data->add(nextBCP->id_, nextBCP->x_, nextBCP->y_);

    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = nextBCP->id_;
    cmh->last_hop_ = my_id_;

    iph->daddr() = nextBCP->id_;

    send(p, 0);
}

/*----------------BCP DETECTION------------------------*/
bool HACHAgent::checkBCP(node *pNode) {
    int count = 0;
    for (node *temp = neighbor_list_; temp; temp = temp->next_) {
        double d = G::distance(temp, pNode);
        if (fabs(d - sensor_range_) < EPSILON) continue;
        else if (d < sensor_range_) {
            count++;
        }

        if (count >= degree_coverage_) return false;
    }

    return true;
}

node *HACHAgent::getBCP(Point point) {
    for (node *tmp = bcp_list; tmp; tmp = tmp->next_)
        if (fabs(tmp->x_ - point.x_) < EPSILON && fabs(tmp->y_ - point.y_) < EPSILON) return tmp;
    return NULL;
}

void HACHAgent::updateBCP(node *pNode, node *compare) {
    neighbor *n = getNeighbor(pNode->id_);
    Angle angle = G::angle(this, n, this, pNode);
    Angle angle1 = G::angle(this, compare, this, pNode);
    if (angle1 > angle) {
        pNode->id_ = compare->id_;
    }
}

void HACHAgent::bcpDetection() {
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
            updateBCP(temp, n);
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
            updateBCP(temp, n);
        }
    }
}

/*----------------Coverage Hole DETECTION------------------------*/
node *HACHAgent::getNextBCP(node *pNode) {
    neighbor *n = getNeighbor(pNode->id_);
    if (G::angle(this, pNode, this, n) >= M_PI) {
        node *bcp = getBCP(*pNode);
        node *next = bcp->next_ == NULL ? bcp_list : bcp->next_;
        neighbor *next_n = getNeighbor(next->id_);
        if (G::angle(this, next, this, next_n) >= M_PI) return bcp;
        else return next;
    }

    return NULL;
}

void HACHAgent::holeBoundaryDetection() {
    if (bcp_list == NULL) return;
    Packet *p;
    hdr_cmn *cmh;
    hdr_ip *iph;
    hdr_hach *ch;

    for (node *n = bcp_list; n; n = n->next_) {
        node *nextBCP = getNextBCP(n);
        if (nextBCP != NULL) continue;

        p = allocpkt();

        HACHPacketData *chpkt_data = new HACHPacketData();
        chpkt_data->add(n->id_, n->x_, n->y_);
        p->setdata(chpkt_data);

        cmh = HDR_CMN(p);
        iph = HDR_IP(p);
        ch = HDR_HACH(p);

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

/*----------------Utils function----------------------*/
void
HACHAgent::startUp() {
    FILE *fp;
    fp = fopen("BCPCoverageHole.tr", "w");
    fclose(fp);
    fp = fopen("PatchingHole.tr", "a");
    fclose(fp);
}

void HACHAgent::addNeighbor(nsaddr_t nid, Point location) {
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

/*-------------------------- DUMP --------------*/
void HACHAgent::dumpCoverageBoundhole(polygonHole *hole) {
    FILE *fp;
    fp = fopen("BCPCoverageHole.tr", "a");
    for (node *temp = hole->node_list_; temp; temp = temp->next_) {
        fprintf(fp, "%d\t%f\t%f\n", temp->id_, temp->x_, temp->y_);
    }
    fprintf(fp, "\n");
    fclose(fp);
}

void HACHAgent::dumpPatchingHole(Point p) {
    FILE *fp;
    fp = fopen("PatchingHole.tr", "a");
    fprintf(fp, "%f\t%f\n", p.x_, p.y_);
    fclose(fp);
}

void HACHAgent::recvHACH(Packet *p) {
    hdr_cmn *cmh = HDR_CMN(p);;
    hdr_ip *iph = HDR_IP(p);
    hdr_hach *ch = HDR_HACH(p);

    if (iph->saddr() == my_id_) {
        drop(p, "HPA_FINISH");
        runTimeCounter.finish();
        return;
    }

    Point cp = ch->cp_;
    sensor_neighbor *lasthop = (sensor_neighbor *) getNeighbor(cmh->last_hop_);
    node last;
    last.x_ = lasthop->i2_.x_;
    last.y_ = lasthop->i2_.y_;
    last.id_ = lasthop->id_;
    node *next = getNextBCP(&last);
    Point a, b;

    bool inside1 = G::distance(next, cp) <= sensor_range_ + EPSILON;
    if (!inside1) {
        b.x_ = next->x_;
        b.y_ = next->y_;

        Point n1, n2;
        G::circleCircleIntersect(&cp, sensor_range_, this, sensor_range_, &n1, &n2);
        a = n1;

        ch->cp_ = calculatePatchingPoint(a, b);
    }

    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = next->id_;
    cmh->last_hop_ = my_id_;
    iph->daddr() = next->id_;

    send(p, 0);
}

void HACHAgent::sendHACH(node lasthop, node nexthop) {
    Packet *p = allocpkt();
    hdr_cmn *cmh;
    hdr_ip *iph;
    hdr_hach *ch;

    cmh = HDR_CMN(p);
    iph = HDR_IP(p);
    ch = HDR_HACH(p);

    cmh->ptype() = PT_HACH;
    cmh->direction() = hdr_cmn::DOWN;
    cmh->size() += IP_HDR_LEN + ch->size();
    cmh->next_hop_ = nexthop.id_;
    cmh->last_hop_ = my_id_;
    cmh->addr_type_ = NS_AF_INET;

    iph->saddr() = my_id_;
    iph->daddr() = nexthop.id_;
    iph->sport() = RT_PORT;
    iph->dport() = RT_PORT;
    iph->ttl_ = limit_hop;            // more than ttl_ hop => boundary => remove

    Point a, b;
    a.x_ = lasthop.x_;
    a.y_ = lasthop.y_;
    b.x_ = nexthop.x_;
    b.y_ = nexthop.y_;
    ch->cp_ = calculatePatchingPoint(a, b);

    send(p, 0);
}

Point HACHAgent::calculatePatchingPoint(Point a, Point b) {
    Point cp;

    Line ab = G::line(a, b);
    Point midpoint = G::midpoint(a, b);
    Line pline = G::perpendicular_line(midpoint, ab);
    Point tmp;
    if (pline.a_ == 0) {
        tmp.x_ = 0;
        tmp.y_ = -pline.c_ / pline.b_;
    } else {
        tmp.y_ = 0;
        tmp.x_ = -pline.c_ / pline.a_;
    }

    Point n1, n2;
    G::circleLineIntersect(a, sensor_range_, midpoint, tmp, &n1, &n2);

    if (G::orientation(a, n1, b) == 1) {
        cp = n1;
        dumpPatchingHole(n1);
        printf("NewPointX:%fNewPointY:%f\n", n1.x_, n1.y_);
    } else {
        cp = n2;
        dumpPatchingHole(n2);
        printf("NewPointX:%fNewPointY:%f\n", n2.x_, n2.y_);
    }

    return cp;
}
