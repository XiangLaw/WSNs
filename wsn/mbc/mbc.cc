#include "mbc.h"
#include "mbc_packet.h"
#include "mbc_packet_data.h"
#include <vector>

int hdr_mbc::offset_;

/*
 * Mbc Header Class
 */
static class MbcHeaderClass : public PacketHeaderClass {
public:
    MbcHeaderClass() : PacketHeaderClass("PacketHeader/MBC", sizeof(hdr_mbc)) {
        bind_offset(&hdr_mbc::offset_);
    }

    ~MbcHeaderClass() { }
} class_mbchdr;

/*
 * Coverage Agent Class
 */
static class MbcAgentClass : public TclClass {
public:
    MbcAgentClass() : TclClass("Agent/MBC") { }

    TclObject *create(int, const char *const *) {
        return (new MbcAgent());
    }
} class_mbc;

void
MbcTimer::expire(Event *e) {
    ((MbcAgent *) a_->*firing_)();
}

// ------------------------ Agent ------------------------ //
MbcAgent::MbcAgent() : GPSRAgent(),
                       boundhole_timer_(this,
                                        &MbcAgent::holeBoundaryDetection) {
    hole_list_ = NULL;
    boundhole_node_list_ = NULL;
    cover_neighbors_ = NULL;
    sensor_neighbor_list_ = NULL;
    isBoundary = false;

    bind("range_", &communication_range_);
    bind("limit_boundhole_hop_", &limit_hop_);
    sensor_range_ = 0.5 * communication_range_;
}

int MbcAgent::command(int argc, const char *const *argv) {
    if (strcasecmp(argv[1], "start") == 0) {
        startUp();
    } else if (strcasecmp(argv[1], "coverage") == 0) {
        runTimeCounter.start();
        boundaryNodeDetection();
        boundhole_timer_.resched(10 + randSend_.uniform(0.0, 5));
        return TCL_OK;
    }

    return GPSRAgent::command(argc, argv);
}

void MbcAgent::recv(Packet *p, Handler *h) {
    struct hdr_cmn *cmh = HDR_CMN(p);

    switch (cmh->ptype()) {
        case PT_MBC:
            recvCoverage(p);
            break;
        default:
            GPSRAgent::recv(p, h);
    }
}

void MbcAgent::recvCoverage(Packet *p) {
    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_cmn *cmh = HDR_CMN(p);

    MbcPacketData *data = (MbcPacketData *) p->userdata();

    // if the boundhole packet has came back to the initial node
    if (iph->saddr() == my_id_) {
        if (data->size() < 3) {
            drop(p);
            return;
        }

        node firstNode = data->get_intersect_data(0);
        node secondNode = data->get_intersect_data(1);
        if (firstNode.id_ == cmh->last_hop_ && secondNode.id_ == my_id_) {
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
            patchingHole(newHole);

            drop(p, "COVERAGE_BOUNDHOLE");
            runTimeCounter.finish();
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

    node *nb = getNextSensorNeighbor(cmh->last_hop_);
    if (nb == NULL) {
        drop(p, DROP_RTR_NO_ROUTE);
        return;
    }

    sensor_neighbor *n = getSensorNeighbor(cmh->last_hop_);
    data->add(n->id_, n->i2_.x_, n->i2_.y_, n->x_, n->y_);

    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = nb->id_;
    cmh->last_hop_ = my_id_;

    iph->daddr() = nb->id_;

    send(p, 0);
}

/*----------------------------BOUNDARY DETECTION------------------------------------------------------*/
bool MbcAgent::boundaryNodeDetection() {
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

void MbcAgent::holeBoundaryDetection() {
    if (isBoundary) {
        Packet *p;
        hdr_cmn *cmh;
        hdr_ip *iph;
        hdr_mbc *ch;

        for (stuckangle *sa = cover_neighbors_; sa; sa = sa->next_) {
            p = allocpkt();

            MbcPacketData *chpkt_data = new MbcPacketData();
            sensor_neighbor *n = getSensorNeighbor(sa->a_->id_);
            chpkt_data->add(n->id_, n->i2_.x_, n->i2_.y_, n->x_, n->y_);
            p->setdata(chpkt_data);

            cmh = HDR_CMN(p);
            iph = HDR_IP(p);
            ch = HDR_MBC(p);

            cmh->ptype() = PT_MBC;
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

            send(p, 0);
        }
    }
}

/*----------------Utils function----------------------*/
void MbcAgent::startUp() {
    FILE *fp;
    fp = fopen("PatchingHole.tr", "w");
    fclose(fp);
}

void MbcAgent::addNeighbor(nsaddr_t addr, Point p) {
    GPSRAgent::addNeighbor(addr, p);

    addSensorNeighbor(addr, p, 2); // fixed add level 2 sensor neighbors
}

void MbcAgent::addSensorNeighbor(nsaddr_t nid, Point location, int level) {
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

sensor_neighbor *MbcAgent::getSensorNeighbor(nsaddr_t addr) {
    for (sensor_neighbor *temp = sensor_neighbor_list_; temp; temp = (sensor_neighbor *) temp->next_) {
        if (temp->id_ == addr) return temp;
    }
    return NULL;
}

node *MbcAgent::getNextSensorNeighbor(nsaddr_t prev_node) {
    for (stuckangle *pair = cover_neighbors_; pair; pair = pair->next_) {
        if (prev_node == pair->a_->id_) return pair->b_;
    }
    return NULL;
}

void MbcAgent::patchingHole(polygonHole *hole) {
    node *tmp = hole->node_list_;
    double min_x, min_y, max_x, max_y;
    min_x = max_x = tmp->x_;
    min_y = max_y = tmp->y_;
    for (tmp = hole->node_list_; tmp; tmp = tmp->next_) {
        if (min_x > tmp->x_)
            min_x = tmp->x_;
        else if (max_x < tmp->x_)
            max_x = tmp->x_;
        if (min_y > tmp->y_)
            min_y = tmp->y_;
        else if (max_y < tmp->y_)
            max_y = tmp->y_;
    }

    double x = 0;
    double y = min_y + sensor_range_ / 2;

    custom_node *patching_list = NULL;
    custom_node *node_tmp = NULL;

    // step 1. optimal deployment in the rectangle
    int i = 1;
    while (y < max_y) {
        if (i % 2 == 1) { // odd line
            x = min_x + sensor_range_ * sqrt(3) / 2;
        } else { // even line
            x = min_x;
        }
        while (x < max_x) {
            addNodeToList(x, y, &patching_list);
            x += sensor_range_ * sqrt(3);
        }
        if ((x - max_x) < sensor_range_ * sqrt(3) / 2)
            addNodeToList(max_x, y, &patching_list);
        i++;
        y += sensor_range_ * 3 / 2;
    }
    if (y - max_y < sensor_range_ / 2) {
        if (i % 2 == 1) { // odd line
            x = min_x + sensor_range_ * sqrt(3) / 2;
        } else { // even line
            x = min_x;
        }
        if ((max_y - y) > sensor_range_ / 2 && (max_y - y) < sensor_range_ * 3 / 2)
            y = max_y;
        while (x < max_x) {
            addNodeToList(x, y, &patching_list);
            x += sensor_range_ * sqrt(3);
        }
        if ((x - max_x) < sensor_range_ * sqrt(3) / 2)
            addNodeToList(max_x, y, &patching_list);
    }

    // step 2 & 3. eliminate nodes located outside
    // note: no use of projection here
    for (node_tmp = patching_list; node_tmp; node_tmp = node_tmp->next_) {
        if (!G::isPointInsidePolygon(node_tmp, hole->node_list_)) {
            eliminateNode(&(*node_tmp), &patching_list, hole);
        }
    }

    // step 4. eliminate redundant nodes
    optimize(&patching_list, hole);

    for (node_tmp = patching_list; node_tmp; node_tmp = node_tmp->next_) {
        dumpPatchingHole(*node_tmp);
    }
}

void MbcAgent::addNodeToList(double x, double y, custom_node **list) {
    custom_node *newNode = new custom_node();
    newNode->x_ = x;
    newNode->y_ = y;
    newNode->next_ = *list;
    newNode->is_removable_ = false;
    *list = newNode;
//    dumpPatchingHole(*newNode);
}

void MbcAgent::dumpPatchingHole(Point point) {
    FILE *fp = fopen("PatchingHole.tr", "a+");
//    fprintf(fp, "%f\t%f\n", point.x_ - sensor_range_ / 2, point.y_ - sensor_range_ * sqrt(3) / 2);
//    fprintf(fp, "%f\t%f\n", point.x_ + sensor_range_ / 2, point.y_ - sensor_range_ * sqrt(3) / 2);
//    fprintf(fp, "%f\t%f\n", point.x_ + sensor_range_, point.y_);
//    fprintf(fp, "%f\t%f\n", point.x_ + sensor_range_ / 2, point.y_ + sensor_range_ * sqrt(3) / 2);
//    fprintf(fp, "%f\t%f\n", point.x_ - sensor_range_ / 2, point.y_ + sensor_range_ * sqrt(3) / 2);
//    fprintf(fp, "%f\t%f\n", point.x_ - sensor_range_, point.y_);
//    fprintf(fp, "%f\t%f\n\n", point.x_ - sensor_range_ / 2, point.y_ - sensor_range_ * sqrt(3) / 2);
    fprintf(fp, "%f\t%f\n", point.x_, point.y_);
    fclose(fp);
}

void MbcAgent::dumpPatchingHole(Point p1, Point p2) {
    FILE *fp = fopen("PatchingHole.tr", "a+");
//    fprintf(fp, "%f\t%f\n", point.x_ - sensor_range_ / 2, point.y_ - sensor_range_ * sqrt(3) / 2);
//    fprintf(fp, "%f\t%f\n", point.x_ + sensor_range_ / 2, point.y_ - sensor_range_ * sqrt(3) / 2);
//    fprintf(fp, "%f\t%f\n", point.x_ + sensor_range_, point.y_);
//    fprintf(fp, "%f\t%f\n", point.x_ + sensor_range_ / 2, point.y_ + sensor_range_ * sqrt(3) / 2);
//    fprintf(fp, "%f\t%f\n", point.x_ - sensor_range_ / 2, point.y_ + sensor_range_ * sqrt(3) / 2);
//    fprintf(fp, "%f\t%f\n", point.x_ - sensor_range_, point.y_);
//    fprintf(fp, "%f\t%f\n\n", point.x_ - sensor_range_ / 2, point.y_ - sensor_range_ * sqrt(3) / 2);
    fprintf(fp, "%f\t%f\n", p1.x_, p1.y_);
    fprintf(fp, "%f\t%f\n\n", p2.x_, p2.y_);
    fclose(fp);
}

void MbcAgent::eliminateNode(custom_node *n, custom_node **list, polygonHole *hole) {
    for (node *tmp = hole->node_list_; tmp; tmp = tmp->next_) {
        node *tmp2 = tmp->next_ == NULL ? hole->node_list_ : tmp->next_;
        Point i1, i2;
        if (G::circleLineIntersect(*n, sensor_range_, *tmp, *tmp2, &i1, &i2) > 0) {
            n->is_removable_ = true;
            return;
        }
    }
    removeNodeFromList(n, &(*list));
}

void MbcAgent::removeNodeFromList(custom_node *n, custom_node **list) {
    custom_node *previous = NULL;
    for (custom_node *tmp = *list; tmp; tmp = tmp->next_) {
        if (tmp->x_ == n->x_ && tmp->y_ == n->y_) {
            if (previous) {
                previous->next_ = tmp->next_;
            } else {
                *list = tmp->next_;
            }
            delete tmp;
            return;
        }
        previous = tmp;
    }
}

void MbcAgent::optimize(custom_node **list, polygonHole *hole) {
    // due to a lot of for loops
    // do it manually :v
}
