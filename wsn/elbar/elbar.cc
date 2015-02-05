#include <bits/ios_base.h>
#include <X11/Xutil.h>
#include "elbar.h"
#include "elbar_packet.h"
#include "elbar_packet_data.h"

int hdr_elbar_grid::offset_;

static class ElbarGridOfflineHeaderClass : public PacketHeaderClass {
public:
    ElbarGridOfflineHeaderClass() : PacketHeaderClass("PacketHeader/ELBARGRIDOFFLINE", sizeof(hdr_elbar_grid)) {
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


/**
* Agent implementation
*/
ElbarGridOfflineAgent::ElbarGridOfflineAgent() : GridOfflineAgent() {
    this->alpha_max_ = M_PI * 2 / 3;
    this->alpha_min_ = M_PI / 3;

    hole_list_ = NULL;
}

char const* ElbarGridOfflineAgent::getAgentName() {
    return "ElbarGirdOffline";
}

int
ElbarGridOfflineAgent::command(int argc, const char *const *argv) {
    if (argc == 2) {
        if (strcasecmp(argv[1], "broadcast") == 0) {
            broadcastHci();
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
            printf("%d - PT_Elbar\n", my_id_);
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
            printf("%d - broadcast recv\n", my_id_);
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
bool ElbarGridOfflineAgent::detectParallelogram() {
    struct polygonHole *tmp;
    struct node *item;

    struct node *ai;
    struct node *aj;

    struct node a;
    struct node b;
    struct node c;

    struct node *vi;
    struct node *vj;

    double hi;
    double hj;
    double h;

    Line li;
    Line lj;

    double angle;
    /*// detect view angle
    for (tmp = hole_list_; tmp != NULL; tmp = tmp->next_){
        item = tmp->node_list_;
        struct node* left = tmp->node_list_;
        struct node* right = tmp->node_list_;
        if (item != NULL && item->next_ != NULL) {
            item = item->next_;
            do {
                Angle a = G::rawAngle(left, this, item, this);
                if (a < 0) left = item;
                a = G::rawAngle(right, this, item, this);
                if (a > 0) right = item;
                item = item->next_;
            } while (item && item != tmp->node_list_);
        }
    }*/
    printf("%d\t- Detect Parallelogram\n", my_id_);

    // detect view angle
    ai = hole_list_->node_list_;
    aj = hole_list_->node_list_;
    tmp = hole_list_;
    item = tmp->node_list_;
    for (tmp = hole_list_; tmp != NULL; tmp = tmp->next_) {
        if (G::directedAngle(ai, this, item) > 0)
            ai = item;
        if (G::directedAngle(aj, this, item) < 0)
            aj = item;
        item = item->next_;
    }

    // detect parallelogram
    vi = hole_list_->node_list_;
    vj = hole_list_->node_list_;
    hi = 0;
    hj = 0;
    tmp = hole_list_;
    item = tmp->node_list_; // A(k)

    for (tmp = hole_list_; tmp != NULL; tmp = tmp->next_) {
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
    }

    li = G::parallel_line(vi, G::line(this, ai));
    lj = G::parallel_line(vj, G::line(this, aj));

    if (!G::intersection(lj, G::line(this, ai), a) ||
            !G::intersection(li, G::line(this, aj), c) ||
            !G::intersection(li, lj, b))
        return false;

    /*
    save Hole information into local memory if this node is in region 2
     */
    angle = G::angle(ai, this, aj);
    region_ = regionDetermine(angle);
    if (REGION_2 == region_) {
        this->parallelogram_->a_ = a;
        this->parallelogram_->b_ = b;
        this->parallelogram_->c_ = c;
        this->parallelogram_->p_.x_ = this->x_;
        this->parallelogram_->p_.y_ = this->y_;
        this->alpha_ = angle;
    }

    return true;
}

RoutingMode ElbarGridOfflineAgent::holeAvoidingProb() {
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
    RoutingMode routing_mode;

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

//    printf("%d - Broadcast Hole\n", my_id_);
    if (hole_list_ == NULL)
        return;

    p = allocpkt();

    // detect parallelogram
    detectParallelogram();

    payload = new ElbarGridOfflinePacketData();
    for (tmp = hole_list_; tmp != NULL; tmp = tmp->next_) {
        node *item = tmp->node_list_;
        if (item != NULL)
            payload->add_data(item->x_, item->y_);
    }
    p->setdata(payload);

    cmh = HDR_CMN(p);
    iph = HDR_IP(p);
    egh = HDR_ELBAR_GRID(p);

    cmh->ptype() = PT_ELBARGRID;
    cmh->direction() = hdr_cmn::DOWN;
    cmh->size() += IP_HDR_LEN + egh->size();
    cmh->next_hop_ = IP_BROADCAST;
    cmh->last_hop_ = my_id_;
    cmh->addr_type_ = NS_AF_INET;

    iph->saddr() = my_id_;
    iph->daddr() = IP_BROADCAST;
    iph->sport() = RT_PORT;
    iph->dport() = RT_PORT;
    iph->ttl_ = 100;

    egh->forwarding_mode_ = GREEDY_MODE;
//    egh->anchor_point_ = NULL;
    egh->type_ = ELBAR_BROADCAST;
    send(p, 0);
}

void ElbarGridOfflineAgent::sendElbar(Packet *p) {
    hdr_cmn *cmh = HDR_CMN(p);
    hdr_ip *iph = HDR_IP(p);

    cmh->direction() = hdr_cmn::DOWN;
    cmh->addr_type() = NS_AF_INET;
    cmh->last_hop_ = my_id_;
    cmh->next_hop_ = IP_BROADCAST;
    cmh->ptype() = PT_ELBARGRID;
    cmh->num_forwards() = 0;

    iph->saddr() = my_id_;
    iph->daddr() = IP_BROADCAST;
    iph->sport() = RT_PORT;
    iph->dport() = RT_PORT;
    iph->ttl_ = IP_DEF_TTL;

    send(p, 0);
}

void ElbarGridOfflineAgent::recvHci(Packet *p) {
    struct hdr_ip *iph = HDR_IP(p);

    ElbarGridOfflinePacketData *data = (ElbarGridOfflinePacketData *) p->userdata();

    // if the hci packet has came back to the initial node
    if (iph->saddr() == my_id_) {
        drop(p, "ElbarGridOfflineLoopHCI");
        return;
    }

    if (iph->ttl_-- <= 0) {
        drop(p, DROP_RTR_TTL);
        return;
    }

    // create hole core information
    polygonHole *hole_item = new polygonHole();
    hole_item->node_list_ = NULL;
    hole_item->next_ = hole_list_;
    hole_list_ = hole_item;

    // add node info to hole item
    struct node *item;

    for (int i = 0; i < data->size(); i++) {
        Point n = data->get_data(i);
        item = new node();
        item->x_ = n.x_;
        item->y_ = n.y_;
        item->next_ = hole_item->node_list_;
        hole_item->node_list_ = item;
    }

    // determine region & parallelogram
    detectParallelogram();

    if (REGION_3 == region_) {
        drop(p, "ElbarGridOffline_IsInRegion3");
    }
    else if (REGION_1 == region_ || REGION_2 == region_) {
        sendElbar(p);
    }
}


//------------------BoundHole---------------------------
void
ElbarGridOfflineAgent::recvBoundHole(Packet* p)  {
    // add data to packet
    addData(p);

    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_grid *bhh = HDR_GRID(p);

    // if the grid packet has came back to the initial node
    if (iph->saddr() == my_id_) {
        if (iph->ttl_ > (limit_boundhole_hop_ - 5)) {
            drop(p, " SmallHole");    // drop hole that have less than 5 hop
        }
        else {
            createPolygonHole(p);

            dumpBoundhole();
            dumpTime();
            dumpEnergy();
            dumpArea();

            drop(p, " GRID");
        }
        return;
    }

    if (iph->ttl_-- <= 0) {
        drop(p, DROP_RTR_TTL);
        return;
    }

    node *nb = getNeighborByBoundhole(&bhh->prev_, &bhh->last_);

    // no neighbor to forward, drop message. it means the network is not interconnected
    if (nb == NULL) {
        drop(p, DROP_RTR_NO_ROUTE);
        return;
    }

    // if neighbor already send grid message to that node
    if (iph->saddr() > my_id_) {
        for (stuckangle *sa = stuck_angle_; sa; sa = sa->next_) {
            if (sa->a_->id_ == nb->id_) {
                drop(p, " REPEAT");
                return;
            }
        }
    }

    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = nb->id_;
    cmh->last_hop_ = my_id_;

    iph->daddr() = nb->id_;

    bhh->last_ = bhh->prev_;
    bhh->prev_ = *this;

    send(p, 0);
}