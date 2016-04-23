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
HACHAgent::HACHAgent() : GPSRAgent(),
                         boundhole_timer_(this,
                                          &HACHAgent::holeBoundaryDetection) {
    hole_list_ = NULL;
    boundhole_node_list_ = NULL;
    cover_neighbors_ = NULL;
    sensor_neighbor_list_ = NULL;
    isBoundary = false;

    bind("range_", &communication_range_);
    bind("limit_boundhole_hop_", &limit_hop_);
    sensor_range_ = 0.5 * communication_range_;
}

int HACHAgent::command(int argc, const char *const *argv) {
    if (strcasecmp(argv[1], "start") == 0) {
        startUp();
    } else if (strcasecmp(argv[1], "coverage") == 0) {
        boundaryNodeDetection();
        boundhole_timer_.resched(10 + 0.02 * my_id_);
        return TCL_OK;
    }

    return GPSRAgent::command(argc, argv);
}

void HACHAgent::recv(Packet *p, Handler *h) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_hach *ch = HDR_HACH(p);

    switch (cmh->ptype()) {
        case PT_HACH:
            switch (ch->type_) {
                case HACH_BOUNDHOLE:
                    recvCoverage(p);
                    break;
                case HACH_HACH:
                    recvHACH(p);
                    break;
                default:
                    drop(p, "UnknownType");
            }
            break;
        default:
            GPSRAgent::recv(p, h);
    }
}

void HACHAgent::recvCoverage(Packet *p) {
    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_cmn *cmh = HDR_CMN(p);

    HACHPacketData *data = (HACHPacketData *) p->userdata();

    // if the boundhole packet has came back to the initial node
    if (iph->saddr() == my_id_) {
        if (data->size() < 3) {
            drop(p);
            return;
        }

        node firstNode = data->get_intersect_data(0);
        node secondNode = data->get_intersect_data(1);
        if (firstNode.id_ == cmh->last_hop_ && secondNode.id_ == my_id_) {
            node nexthop = secondNode;
            nexthop.id_ = getNextSensorNeighbor(cmh->last_hop_)->id_;
            node lasthop = firstNode;
            lasthop.id_ = my_id_;

            node *intersect_head = NULL;
            node *node_head = NULL;

            for (int i = 0; i < data->size(); i++) {
                node n_intersect = data->get_intersect_data(i);
                node *intersect = new node();
                intersect->x_ = n_intersect.x_;
                intersect->y_ = n_intersect.y_;
                intersect->id_ = n_intersect.id_;
                intersect->next_ = intersect_head;
                intersect_head = intersect;

                node n_node = data->get_node_data(i);
                node *node_ = new node();
                node_->x_ = n_node.x_;
                node_->y_ = n_node.y_;
                node_->id_ = n_node.id_;
                node_->next_ = node_head;
                node_head = node_;
            }

            polygonHole *newHole = new polygonHole();
            newHole->node_list_ = intersect_head;
            newHole->next_ = hole_list_;
            hole_list_ = newHole;

            polygonHole *newNodeHole = new polygonHole();
            newNodeHole->node_list_ = node_head;
            newNodeHole->next_ = boundhole_node_list_;
            boundhole_node_list_ = newNodeHole;
            data->dump();
            drop(p, "HACH_COVERAGE");
            sendHACH(lasthop, nexthop);
            return;
        }
    }

    if (iph->ttl_-- <= 0) {
        drop(p, DROP_RTR_TTL);
        return;
    }

    if (iph->saddr() < my_id_) {
        drop(p, "REPEAT");
        return;
    }

    node *nexthop = getNextSensorNeighbor(cmh->last_hop_);
    if (nexthop == NULL) {
        drop(p, DROP_RTR_NO_ROUTE);
        return;
    }

    sensor_neighbor *n = getSensorNeighbor(cmh->last_hop_);
    data->add(n->id_, n->i2_.x_, n->i2_.y_, n->x_, n->y_);

    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = nexthop->id_;
    cmh->last_hop_ = my_id_;

    iph->daddr() = nexthop->id_;

    send(p, 0);
}

/*----------------------------BOUNDARY DETECTION------------------------------------------------------*/
bool HACHAgent::boundaryNodeDetection() {
    bool isOnBoundary = false;
    if (sensor_neighbor_list_ == NULL) return true;
    if (sensor_neighbor_list_->next_ == NULL) {
        cover_neighbors_ = new stuckangle();
        cover_neighbors_->a_ = sensor_neighbor_list_;
        cover_neighbors_->b_ = sensor_neighbor_list_;
        cover_neighbors_->next_ = NULL;
        return true;
    }

    for (sensor_neighbor *temp = sensor_neighbor_list_; temp; temp = (sensor_neighbor *) temp->next_) {
        sensor_neighbor *next = temp->next_ == NULL ? sensor_neighbor_list_ : (sensor_neighbor *) temp->next_;

        if (G::distance(temp->i2_, next) - sensor_range_ > EPSILON) {
            isOnBoundary = true;
            stuckangle *p = new stuckangle();
            p->a_ = temp;
            p->b_ = next;
            p->next_ = NULL;

            if (cover_neighbors_ == NULL) {
                cover_neighbors_ = p;
            } else {
                cover_neighbors_->next_ = p;
            }
        }
    }

    this->isBoundary = isOnBoundary;
    return isOnBoundary;
}

void HACHAgent::holeBoundaryDetection() {
    if (isBoundary) {
        Packet *p;
        hdr_cmn *cmh;
        hdr_ip *iph;
        hdr_hach *ch;

        for (stuckangle *sa = cover_neighbors_; sa; sa = sa->next_) {
            p = allocpkt();

            HACHPacketData *chpkt_data = new HACHPacketData();
            sensor_neighbor *n = getSensorNeighbor(sa->a_->id_);
            chpkt_data->add(n->id_, n->i2_.x_, n->i2_.y_, n->x_, n->y_);
            p->setdata(chpkt_data);

            cmh = HDR_CMN(p);
            iph = HDR_IP(p);
            ch = HDR_HACH(p);

            cmh->ptype() = PT_HACH;
            cmh->direction() = hdr_cmn::DOWN;
            cmh->size() += IP_HDR_LEN + ch->size() + chpkt_data->data_len_;
            cmh->next_hop_ = sa->b_->id_;
            cmh->last_hop_ = my_id_;
            cmh->addr_type_ = NS_AF_INET;

            iph->saddr() = my_id_;
            iph->daddr() = sa->b_->id_;
            iph->sport() = RT_PORT;
            iph->dport() = RT_PORT;
            iph->ttl_ = limit_hop_;            // more than ttl_ hop => boundary => remove

            ch->type_ = HACH_BOUNDHOLE;

            send(p, 0);
        }
    }
}

/*----------------Utils function----------------------*/
void HACHAgent::startUp() {
    FILE *fp;
    fp = fopen("PatchingHole.tr", "w");
    fclose(fp);
}

void HACHAgent::addNeighbor(nsaddr_t addr, Point p) {
    GPSRAgent::addNeighbor(addr, p);

    addSensorNeighbor(addr, p, 2); // fixed add level 2 sensor neighbors
}

void HACHAgent::addSensorNeighbor(nsaddr_t nid, Point location, int level) {
    sensor_neighbor *temp = getSensorNeighbor(nid);

    if (temp == NULL)            // it is a new node
    {
        if (G::distance(this, location) > level * sensor_range_) return;

        temp = new sensor_neighbor;
        temp->id_ = nid;
        temp->x_ = location.x_;
        temp->y_ = location.y_;
        temp->time_ = Scheduler::instance().clock();
        G::circleCircleIntersect(this, sensor_range_, temp, sensor_range_, &temp->i1_, &temp->i2_);
        temp->next_ = NULL;


        if (sensor_neighbor_list_ == NULL)        // the list now is empty
        {
            sensor_neighbor_list_ = temp;
        }
        else    // the nodes list is not empty
        {
            Angle angle = G::angle(*this, sensor_neighbor_list_->i1_, *this, temp->i1_);
            sensor_neighbor *i, *next, *i2 = NULL;

            for (i = sensor_neighbor_list_; i->next_; i = (sensor_neighbor *) i->next_) {
                next = (sensor_neighbor *) i->next_;
                double a = G::angle(*this, sensor_neighbor_list_->i1_, *this, next->i1_);
                if (a == angle) {
                    Angle angle2 = G::angle(*this, temp->i1_, *this, temp->i2_);
                    if (G::angle(*this, next->i1_, *this, next->i2_) < angle2) {
                        continue;
                    } else {
                        temp->next_ = i->next_;
                        i->next_ = temp;
                        break;
                    }
                }

                if (a > angle) {
                    Angle ai_ = G::angle(*this, i->i1_, *this, i->i2_);
                    if (G::angle(*this, i->i1_, *this, temp->i1_) < ai_
                        && G::angle(*this, i->i1_, *this, temp->i2_) < ai_) {
                        break;
                    } else {
                        i->next_ = temp;
                        Angle atemp_ = G::angle(*this, temp->i1_, *this, temp->i2_);
                        for (i2 = next; i2; i2 = (sensor_neighbor *) i2->next_) {
                            if (G::angle(*this, temp->i1_, *this, i2->i1_) < atemp_
                                && G::angle(*this, temp->i1_, *this, i2->i2_) < atemp_) {
                                continue;
                            } else {
                                break;
                            }
                        }

                        temp->next_ = i2;
                        if (i2 == NULL) {
                            for (i2 = sensor_neighbor_list_; i2; i2 = (sensor_neighbor *) i2->next_) {
                                if (G::angle(*this, temp->i1_, *this, i2->i1_) < atemp_
                                    && G::angle(*this, temp->i1_, *this, i2->i2_) < atemp_) {
                                    continue;
                                } else {
                                    break;
                                }
                            }
                            sensor_neighbor_list_ = i2;
                        }
                        break;
                    }
                }
            }

            if (i->next_ == NULL) // if angle is maximum, add temp to end of neighobrs list
            {
                Angle ai_ = G::angle(*this, i->i1_, *this, i->i2_);
                if (G::angle(*this, i->i1_, *this, temp->i1_) < ai_
                    && G::angle(*this, i->i1_, *this, temp->i2_) < ai_) {
                } else {
                    i->next_ = temp;
                    Angle atemp_ = G::angle(*this, temp->i1_, *this, temp->i2_);
                    for (i2 = sensor_neighbor_list_; i2; i2 = (sensor_neighbor *) i2->next_) {
                        if (G::angle(*this, temp->i1_, *this, i2->i1_) < atemp_
                            && G::angle(*this, temp->i1_, *this, i2->i2_) < atemp_) {
                            continue;
                        } else {
                            break;
                        }
                    }
                    sensor_neighbor_list_ = i2;
                }
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

sensor_neighbor *HACHAgent::getSensorNeighbor(nsaddr_t addr) {
    for (sensor_neighbor *temp = sensor_neighbor_list_; temp; temp = (sensor_neighbor *) temp->next_) {
        if (temp->id_ == addr) return temp;
    }
    return NULL;
}

node *HACHAgent::getNextSensorNeighbor(nsaddr_t prev_node) {
    for (stuckangle *pair = cover_neighbors_; pair; pair = pair->next_) {
        if (prev_node == pair->a_->id_) return pair->b_;
    }
    return NULL;
}

/*-------------------------- DUMP --------------*/
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
        return;
    }

    Point cp = ch->cp_;
    node next;
    next.id_ = getNextSensorNeighbor(cmh->last_hop_)->id_;
    sensor_neighbor *neighbor = getSensorNeighbor(next.id_);
    next.x_ = neighbor->i1_.x_;
    next.y_ = neighbor->i1_.y_;

    Point a, b;

    bool inside1 = G::distance(next, cp) <= sensor_range_ + EPSILON;
    if (!inside1) {
        b.x_ = next.x_;
        b.y_ = next.y_;

        Point n1, n2;
        G::circleCircleIntersect(&cp, sensor_range_, this, sensor_range_, &n1, &n2);
        a = n1;

        ch->cp_ = calculatePatchingPoint(a, b);
    }

    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = next.id_;
    cmh->last_hop_ = my_id_;
    iph->daddr() = next.id_;

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
    iph->ttl_ = limit_hop_;            // more than ttl_ hop => boundary => remove

    Point a, b;
    a.x_ = lasthop.x_;
    a.y_ = lasthop.y_;
    b.x_ = nexthop.x_;
    b.y_ = nexthop.y_;
    ch->cp_ = calculatePatchingPoint(a, b);
    ch->type_ = HACH_HACH;

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
    G::circleLineIntersect(a, sensor_range_ - 0.01, midpoint, tmp, &n1, &n2);

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
