//
// Created by eleven on 10/7/15.
//

#include "bcpcoverage.h"
#include "bcpcoverage_packet_data.h"

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
    } else if (strcasecmp(argv[1], "coverage") == 0) {
        runTimeCounter.start();
        bcpDetection();
        // dumpBoundaryDetect();
        boundhole_timer_.resched(randSend_.uniform(10.0, 30.0));
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
        for (int i = data->size() - 1; i >= 0; i--) {
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
        drop(p, "BCPCOVERAGE");
        runTimeCounter.finish();
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
bool BCPCoverageAgent::checkBCP(node *pNode) {
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

node *BCPCoverageAgent::getBCP(Point point) {
    for (node *tmp = bcp_list; tmp; tmp = tmp->next_)
        if (fabs(tmp->x_ - point.x_) < EPSILON && fabs(tmp->y_ - point.y_) < EPSILON) return tmp;
    return NULL;
}

void BCPCoverageAgent::updateBCP(node *pNode, node *compare) {
    neighbor *n = getNeighbor(pNode->id_);
    Angle angle = G::angle(this, n, this, pNode);
    Angle angle1 = G::angle(this, compare, this, pNode);
    if (angle1 > angle) {
        pNode->id_ = compare->id_;
    }
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
node *BCPCoverageAgent::getNextBCP(node *pNode) {
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

void BCPCoverageAgent::holeBoundaryDetection() {
    if (bcp_list == NULL) return;
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

/*----------------Utils function----------------------*/
void
BCPCoverageAgent::startUp() {
    FILE *fp;
    fp = fopen("BCPCoverageHole.tr", "w");
    fclose(fp);
    fp = fopen("PatchingHole.tr", "a");
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
    double r_ = sensor_range_ - 0.01;

    node *bi = list;
    double shortest_distance = G::distance(list, list->next_);
    for (node *tmp = list->next_; tmp; tmp = tmp->next_) {
        double d = G::distance(tmp, tmp->next_ == NULL ? list : tmp->next_);
        if (shortest_distance > d) {
            bi = tmp;
            shortest_distance = d;
        }
    }

    bool flag = true;
    Point patching_point, n0_inside, n0_outside, p1, p2;

    node *next_bi = bi->next_ == NULL ? list : bi->next_;
    node *bj = next_bi->next_ == NULL ? list : next_bi->next_;
    G::circleCircleIntersect(bi, r_, next_bi, r_, &patching_point, &n0_outside);

    while (flag && G::distance(bi, bj) <= 2 * r_) {
        // detect N0
        G::circleCircleIntersect(bi, r_, bj, r_, &n0_inside, &n0_outside);

        if (n0_inside == n0_outside) {
            for (node *i = next_bi; i != bj; i = i->next_ == NULL ? list : i->next_) {
                if (G::distance(n0_inside, i) > r_) {
                    flag = false;
                    break;
                }
            }
        } else {
            for (node *i = next_bi; i != bj; i = i->next_ == NULL ? list : i->next_) {
                if (n0_inside == n0_outside) {
                    if (G::distance(n0_inside, i) > r_) {
                        flag = false;
                        break;
                    }
                } else {
                    int re = G::circleLineIntersect(*i, r_, n0_inside, n0_outside, &p1, &p2);
                    if (re == 2) {
                        if (!G::segmentAggregation(&n0_inside, &n0_outside, &p1, &p2)) {
                            flag = false;
                            break;
                        }
                    } else if (re == 1) {
                        if (G::onSegment(n0_inside, p1, n0_outside)) {
                            n0_inside = n0_outside = p1;
                        } else {
                            flag = false;
                            break;
                        }
                    } else {
                        flag = false;
                        break;
                    }
                }

            }
        }

        if (flag) {
            if (G::angle(*bi, n0_inside, *bi, n0_outside) > M_PI) {
                patching_point.x_ = n0_outside.x_;
                patching_point.y_ = n0_outside.y_;
            } else {
                patching_point.x_ = n0_inside.x_;
                patching_point.y_ = n0_inside.y_;
            }
        }

        if (bj == bi) break;

        if (flag) {
            bj = bj->next_ == NULL ? list : bj->next_;
        }
    }

    printf("NewPointX:%fNewPointY:%f\n", patching_point.x_, patching_point.y_);
    dumpPatchingHole(patching_point);

    return list;
}

/*-------------------------- DUMP --------------*/
void BCPCoverageAgent::dumpCoverageBoundhole(polygonHole *hole) {
    FILE *fp;
    fp = fopen("BCPCoverageHole.tr", "a");
    for (node *temp = hole->node_list_; temp; temp = temp->next_) {
        fprintf(fp, "%f\t%f\n", temp->x_, temp->y_);
    }
    fprintf(fp, "\n");
    fclose(fp);
}

void BCPCoverageAgent::dumpPatchingHole(Point p) {
    FILE *fp;
    fp = fopen("PatchingHole.tr", "a");
    fprintf(fp, "%f\t%f\n", p.x_, p.y_);
    fclose(fp);
}