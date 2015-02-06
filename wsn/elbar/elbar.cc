#include <bits/ios_base.h>
#include <X11/Xutil.h>
#include "elbar.h"
#include "elbar_packet.h"
#include "elbar_packet_data.h"

int hdr_elbar_grid::offset_;

static class ElbarGridOfflineHeaderClass : public PacketHeaderClass {
public:
    ElbarGridOfflineHeaderClass() : PacketHeaderClass("PacketHeader/ELBARGRID", sizeof(hdr_elbar_grid)) {
        bind_offset(&hdr_elbar_grid::offset_);
    }
} class_elbargridofflinehdr;

static class ElbarGridOfflineAgentClass : public TclClass {
public:
    ElbarGridOfflineAgentClass() : TclClass("Agent/ELBARGRIDOFFLINE") {
    }

    TclObject *create(int argc, const char *const *argv) {
        return (new ElbarGridOfflineAgent());
    }
} class_elbargridoffline;

void
ElbarGridOfflineTimer::expire(Event *e) {
    (a_->*firing_)();
}

/**
* Agent implementation
*/
ElbarGridOfflineAgent::ElbarGridOfflineAgent()
        : GridOfflineAgent(),
          broadcast_timer_(this, &ElbarGridOfflineAgent::broadcastHci), {
    this->alpha_max_ = M_PI * 2 / 3;
    this->alpha_min_ = M_PI / 3;

    hole_list_ = NULL;
    parallelogram_ = NULL;
    alpha_ = NULL;
}

char const *ElbarGridOfflineAgent::getAgentName() {
    return "ElbarGirdOffline";
}

int
ElbarGridOfflineAgent::command(int argc, const char *const *argv) {
    if (argc == 2) {
        if (strcasecmp(argv[1], "broadcast") == 0) {
            broadcast_timer_.resched(randSend_.uniform(0.0, 5));
//            broadcastHci();
            return TCL_OK;
        }
    }

    return GridOfflineAgent::command(argc, argv);
}

// handle the receive packet just of type PT_ELBARGRID
void
ElbarGridOfflineAgent::recv(Packet *p, Handler *h) {
    hdr_cmn *cmh = HDR_CMN(p);

    switch (cmh->ptype()) {
        case PT_ELBARGRID:
            recvElbar(p);
            break;
        default:
            GridOfflineAgent::recv(p, h);
            break;
    }
}

/*------------------------------ Recv -----------------------------*/
void ElbarGridOfflineAgent::recvElbar(Packet *p) {
    hdr_elbar_grid *egh = HDR_ELBAR_GRID(p);

    switch (egh->type_) {
        case ELBAR_BROADCAST:
            recvHci(p);
            break;
        case ELBAR_DATA:
            printf("%d - data recv\n", my_id_);
            routing(p);
            break;
        default:
            printf("%d - drop\n", my_id_);
            drop(p, "UnknowType");
            break;

    }
}

/*------------------------------ Routing --------------------------*/
/*
 * Hole covering parallelogram determination & region determination
 */
void ElbarGridOfflineAgent::detectParallelogram() {
    struct polygonHole *tmp;
    struct node *ai;
    struct node *aj;

    struct node *vi;
    struct node *vj;
    struct node *item;

    double angle;

    for (tmp = hole_list_; tmp; tmp = tmp->next_) {
        struct node a;
        struct node b;
        struct node c;

        double hi;
        double hj;
        double h;

        Line li;
        Line lj;

        // check if node is inside grid
        if (!G::isPointInPolygon(this, tmp->node_list_)) {
            // detect view angle
            ai = tmp->node_list_;
            aj = tmp->node_list_;
            item = tmp->node_list_;

            do {
                if (G::directedAngle(ai, this, item) > 0) {
                    ai = item;
                }
                if (G::directedAngle(aj, this, item) < 0)
                    aj = item;
                item = item->next_;
            } while (item && item->next_ != tmp->node_list_);

            printf("%d - Detect Parallelogram: (%f,%f) ai=(%f,%f) aj=(%f,%f)\n", my_id_, x_, y_, ai->x_, ai->y_, aj->x_, aj->y_);

            // detect parallelogram
            vi = tmp->node_list_;
            vj = tmp->node_list_;
            hi = 0;
            hj = 0;

            item = tmp->node_list_;

            do {
                h = G::distance(item->x_, item->y_, this->x_, this->y_, ai->x_, ai->y_);
                if (h > hi) {
                    hi = h;
                    vi = item;
                }
                h = G::distance(item->x_, item->y_, this->x_, this->y_, aj->x_, aj->y_);
                if (h > hj) {
                    hj = h;
                    vj = item;
                }
                item = item->next_;
            } while (item && item->next_ != tmp->node_list_);

            li = G::parallel_line(vi, G::line(this, ai));
            lj = G::parallel_line(vj, G::line(this, aj));

            if (!G::intersection(lj, G::line(this, ai), &a) ||
                    !G::intersection(li, G::line(this, aj), &c) ||
                    !G::intersection(li, lj, &b)) {
                printf("%d - fail\n", my_id_);
                continue;
            }

            /*
        save Hole information into local memory if this node is in region 2
         */
            angle = G::directedAngle(aj, this, ai);
            angle = angle > 0 ? angle : -angle; //abs

            region_ = regionDetermine(angle);
            if (REGION_2 == region_) {
                struct parallelogram *parallel = new parallelogram();

                parallel->a_ = a;
                parallel->b_ = b;
                parallel->c_ = c;
                parallel->p_.x_ = this->x_;
                parallel->p_.y_ = this->y_;
                parallel->next_ = this->parallelogram_;
                this->parallelogram_ = parallel;
            }
        } else {
            // angle = alpha_max_;
            region_ = REGION_1;
        }

        angleView *angle_ = new angleView();
        angle_->hole_id_ = tmp->hole_id_;
        angle_->angle_ = angle;
        angle_->next_ = alpha_;
        alpha_ = angle_;
    }

    dumpAngle();
}

int ElbarGridOfflineAgent::holeAvoidingProb() {
    srand(time(NULL));
    if (rand() % 2 == 0)
        return HOLE_AWARE_MODE;
    return GREEDY_MODE;
}

Elbar_Region ElbarGridOfflineAgent::regionDetermine(double angle) {
    if (angle < alpha_min_) return REGION_3;
    if (angle > alpha_max_) return REGION_1;
    return REGION_2;
}

/*
 * Hole bypass routing
 */
void ElbarGridOfflineAgent::routing(Packet *p) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_elbar_grid *egh = HDR_ELBAR_GRID(p);

    Point *destionantion;
    Point *anchor_point;
    int routing_mode;

    printf("routing start\n");

    if (region_ == REGION_3 || region_ == REGION_1 ||
            hole_list_ == NULL) {
        // greedy mode
//        egh->anchor_point_ = NULL;
        egh->forwarding_mode_ = GREEDY_MODE;
        node *nexthop = getNeighborByGreedy(*dest);
        if (nexthop == NULL) {
            drop(p, DROP_RTR_NO_ROUTE);
            return;
        }
        cmh->direction() = hdr_cmn::DOWN;
        cmh->addr_type() = NS_AF_INET;
        cmh->last_hop_ = my_id_;
        cmh->next_hop_ = nexthop->id_;
        send(p, 0);
    }
    else if (region_ == REGION_2) { // elbar routing
        destionantion = dest;
        anchor_point = &(egh->anchor_point_);
        routing_mode = egh->forwarding_mode_;

        if (dest->x_ || dest->y_) {
            if (cmh->direction() == hdr_cmn::UP &&
                    (this->x_ == destionantion->x_ && this->y_ == destionantion->y_))    // up to destination
            {
                dumpHopcount(p);
                port_dmux_->recv(p, 0);
                return;
            }
            if (routing_mode == HOLE_AWARE_MODE) {
                if (G::directedAngle(destionantion, this, &(parallelogram_->a_)) * G::directedAngle(destionantion, this, &(parallelogram_->c_)) >= 0) { // alpha does not contain D
                    egh->forwarding_mode_ = GREEDY_MODE;
                    node *nexthop = getNeighborByGreedy(*destionantion);
                    if (nexthop == NULL) {
                        drop(p, DROP_RTR_NO_ROUTE);
                        return;
                    }
                    cmh->direction() = hdr_cmn::DOWN;
                    cmh->addr_type() = NS_AF_INET;
                    cmh->last_hop_ = my_id_;
                    cmh->next_hop_ = nexthop->id_;
                    send(p, 0);
                }
                else {
                    node *nexthop = getNeighborByGreedy(*anchor_point);
                    if (nexthop == NULL) {
                        drop(p, DROP_RTR_NO_ROUTE);
                        return;
                    }
                    cmh->direction() = hdr_cmn::DOWN;
                    cmh->addr_type() = NS_AF_INET;
                    cmh->last_hop_ = my_id_;
                    cmh->next_hop_ = nexthop->id_;
                    send(p, 0);
                }
            }
            else {
                if (alpha_ &&
                        G::directedAngle(destionantion, this, &(parallelogram_->a_)) * G::directedAngle(destionantion, this, &(parallelogram_->c_)) < 0) {
                    // alpha contains D
                    routing_mode_ = holeAvoidingProb();
                    if (routing_mode_ == HOLE_AWARE_MODE) {
                        if (G::distance(parallelogram_->p_, parallelogram_->c_) <=
                                G::distance(parallelogram_->p_, parallelogram_->a_)) {
                            // pc <= pa
                            if (G::directedAngle(destionantion, &(parallelogram_->c_), &(parallelogram_->p_)) *
                                    G::directedAngle(destionantion, &(parallelogram_->c_), &(parallelogram_->b_))
                                    >= 0) { // cd does not intersect with the hole
                                egh->anchor_point_ = (parallelogram_->c_);
                            }
                            else {
                                egh->anchor_point_ = (parallelogram_->a_);
                            }
                        }
                        else {
                            if (G::directedAngle(destionantion, &(parallelogram_->a_), &(parallelogram_->p_)) *
                                    G::directedAngle(destionantion, &(parallelogram_->a_), &(parallelogram_->b_))
                                    >= 0) {
                                egh->anchor_point_ = (parallelogram_->a_);
                            }
                            else {
                                egh->anchor_point_ = (parallelogram_->c_);
                            }
                        }

                        egh->forwarding_mode_ = HOLE_AWARE_MODE;
                        node *nexthop = getNeighborByGreedy(*anchor_point);
                        if (nexthop == NULL) {
                            drop(p, DROP_RTR_NO_ROUTE);
                            return;
                        }
                        cmh->direction() = hdr_cmn::DOWN;
                        cmh->addr_type() = NS_AF_INET;
                        cmh->last_hop_ = my_id_;
                        cmh->next_hop_ = nexthop->id_;
                        send(p, 0);
                    }
                    else {
                        node *nexthop = getNeighborByGreedy(*destionantion);
                        if (nexthop == NULL) {
                            drop(p, DROP_RTR_NO_ROUTE);
                            return;
                        }
                        cmh->direction() = hdr_cmn::DOWN;
                        cmh->addr_type() = NS_AF_INET;
                        cmh->last_hop_ = my_id_;
                        cmh->next_hop_ = nexthop->id_;
                        send(p, 0);
                    }
                }
                else {
                    node *nexthop = getNeighborByGreedy(*destionantion);
                    if (nexthop == NULL) {
                        drop(p, DROP_RTR_NO_ROUTE);
                        return;
                    }
                    cmh->direction() = hdr_cmn::DOWN;
                    cmh->addr_type() = NS_AF_INET;
                    cmh->last_hop_ = my_id_;
                    cmh->next_hop_ = nexthop->id_;
                    send(p, 0);
                }
            }
        }
    }
}

/*---------------------- Broacast HCI --------------------------------*/
/**
*
*/
void ElbarGridOfflineAgent::broadcastHci() {

    Packet *p;
    ElbarGridOfflinePacketData *payload;
    hdr_cmn *cmh;
    hdr_ip *iph;
    hdr_elbar_grid *egh;

    polygonHole *tmp;
//
    if (hole_list_ == NULL)
        return;

    printf("%d - Broadcast Hole\n", my_id_);

//    detect parallelogram
    detectParallelogram();

    // send only first hole
//    polygonHole* hole = hole_list_;
//    do{
    p = allocpkt();
    payload = new ElbarGridOfflinePacketData();
    for (tmp = hole_list_; tmp != NULL; tmp = tmp->next_) {
        node *item = tmp->node_list_;
        do {
            payload->add_data(item->x_, item->y_);
            item = item->next_;
        } while (item && item->next_ != tmp->node_list_);
    }
    p->setdata(payload);

    cmh = HDR_CMN(p);
    iph = HDR_IP(p);
    egh = HDR_ELBAR_GRID(p);

    cmh->ptype() = PT_ELBARGRID;
    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = IP_BROADCAST;
    cmh->last_hop_ = my_id_;
    cmh->addr_type_ = NS_AF_INET;
    cmh->size() += IP_HDR_LEN + egh->size();

    iph->daddr() = IP_BROADCAST;
    iph->saddr() = my_id_;
    iph->sport() = RT_PORT;
    iph->dport() = RT_PORT;
    iph->ttl_ = limit_boundhole_hop_;

    egh->forwarding_mode_ = GREEDY_MODE;
    egh->type_ = ELBAR_BROADCAST;

    send(p, 0);
//        hole = hole->next_;
//    } while (hole && hole->next_ != hole_list_);
}

void ElbarGridOfflineAgent::sendElbar(Packet *p) {
    hdr_cmn *cmh = HDR_CMN(p);
    hdr_ip *iph = HDR_IP(p);

    cmh->direction() = hdr_cmn::DOWN;
    cmh->last_hop_ = my_id_;

    iph->ttl_--;

    send(p, 0);
}

void ElbarGridOfflineAgent::recvHci(Packet *p) {
    struct hdr_ip *iph = HDR_IP(p);

    // if the hci packet has came back to the initial node
    if (iph->saddr() == my_id_) {
        drop(p, "ElbarGridOfflineLoopHCI");
        return;
    }

    if (iph->ttl_-- <= 0) {
        drop(p, DROP_RTR_TTL);
        return;
    }

    createGrid(p);

    // check if its alpha already exist
    for (angleView *angle = alpha_; angle; angle = angle->next_) {
        if (angle->hole_id_ == iph->saddr()) {
            drop(p, "HciReceived");
            return;
        }
    }

    // determine region & parallelogram if alpha is not detected
    detectParallelogram();

    if (REGION_3 == region_) {
        drop(p, "ElbarGridOffline_IsInRegion3");
    }
    else if (REGION_1 == region_ || REGION_2 == region_) {
        sendElbar(p);
    }
}

void ElbarGridOfflineAgent::createGrid(Packet *p) {
    struct hdr_ip *iph = HDR_IP(p);
//    hdr_cmn*     cmh = HDR_CMN(p);

    // check if is really receive this hole's information
    for (polygonHole *h = hole_list_; h; h = h->next_) {
        if (h->hole_id_ == iph->saddr())    // already received
        {
            drop(p, "Received");
            return;
        }
    }

    // create hole core information
    polygonHole *hole_item = new polygonHole();
    hole_item->node_list_ = NULL;
    hole_item->hole_id_ = iph->saddr();

    // add node info to hole item
    ElbarGridOfflinePacketData *data = (ElbarGridOfflinePacketData *) p->userdata();
    if (my_id_ == 71)
        int x = my_id_;
    data->dump();

    node *head = NULL;
    for (int i = 1; i <= data->size(); i++) {
        node n = data->get_data(i);
        node *item = new node();
        item->x_ = n.x_;
        item->y_ = n.y_;
        item->next_ = head;
        head = item;
    }

    hole_item->node_list_ = head;
    hole_item->next_ = hole_list_;
    hole_list_ = hole_item;
}

/*---------------------- Dump --------------------------------*/

void ElbarGridOfflineAgent::initTraceFile() {
    FILE *fp;
    fp = fopen("Area.tr", "w");
    fclose(fp);
    fp = fopen("GridOffline.tr", "w");
    fclose(fp);
    fp = fopen("Neighbors.tr", "w");
    fclose(fp);
    fp = fopen("AngleView.tr", "w");
    fclose(fp);
    fp = fopen("Time.tr", "w");
    fclose(fp);
    fp = fopen("ElbarGridOnline.tr", "w");
    fclose(fp);
}

void ElbarGridOfflineAgent::dumpAngle() {
    FILE *fp = fopen("AngleView.tr", "a+");
    angleView *a;
    for (a = alpha_; a; a = a->next_) {
        fprintf(fp, "(%d: %f,%f) - %d (%f)\n", my_id_, x_, y_, a->hole_id_, a->angle_);
    }
    fclose(fp);
}
