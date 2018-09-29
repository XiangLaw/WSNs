#include <trace/cmu-trace.h>
#include <time.h>
#include "edgr.h"

int hdr_beacon::offset_;
int hdr_burst::offset_;
int hdr_edgr::offset_;

/*
 * BEACON Header Class
 */
static class BeaconHeaderClass : public PacketHeaderClass {
public:
    BeaconHeaderClass() : PacketHeaderClass("PacketHeader/BEACON", sizeof(hdr_beacon)) {
        bind_offset(&hdr_beacon::offset_);
    }

    ~BeaconHeaderClass() {}
} class_beacon_hdr;

static class BurstHeaderClass : public PacketHeaderClass {
public:
    BurstHeaderClass() : PacketHeaderClass("PacketHeader/EDGR", sizeof(hdr_burst)) {
        bind_offset(&hdr_burst::offset_);
    }

    ~BurstHeaderClass() {}
} class_burst_hdr;

static class EDGRHeaderClass : public PacketHeaderClass {
public:
    EDGRHeaderClass() : PacketHeaderClass("PacketHeader/EDGR", sizeof(hdr_edgr)) {
        bind_offset(&hdr_edgr::offset_);
    }

    ~EDGRHeaderClass() {}
} class_edgr_hdr;

static class EDGRAgentClass : public TclClass {
public:
    EDGRAgentClass() : TclClass("Agent/EDGR") {}

    TclObject *create(int, const char *const *) {
        return (new EDGRAgent());
    }
} class_edgr;

/*
 * Timer
 */
void BroadcastTimer::expire(Event *e) {
    agent_->forwardBroadcast(packet_);
}

void EDGRBeaconTimer::expire(Event *e) {
    agent_->beacontout();
}

void EDGRAgent::beacontout() {
    sendBeacon();
    if (hello_period_ > 0) beacon_timer_.resched(hello_period_);
}

// ------------------------- Agent ------------------------ //
EDGRAgent::EDGRAgent() : Agent(PT_EDGR), beacon_timer_(this) {
    my_id_ = -1;
    x_ = -1;
    y_ = -1;

    node_ = NULL;
    port_dmux_ = NULL;
    neighbor_list_ = NULL;
    trace_target_ = NULL;

    count_left_ = 0;
    count_right_ = 0;
    gamma_ = 0.5;
    d_opt_ = 25.0;     // 25 m, respect to alpha_ = 2, c1_ = 100, c2_ = 100, c3_ = 60
    d_o_ = 35.4;     // 35.4 m
    is_burst_sent_ = false;
    is_burst_left_came_back_ = false;
    is_burst_right_came_back_ = false;

    dest = new Point();
    bind("hello_period_", &hello_period_);
}

int
EDGRAgent::command(int argc, const char *const *argv) {
    if (argc == 2) {
        if (strcasecmp(argv[1], "start") == 0) {
            startUp();
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "dumpEnergy") == 0) {
            dumpEnergyByTime();
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "dumpBroadcast") == 0) {
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "dump") == 0) {
            dumpNeighbor();
            dumpEnergy();
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "nodeoff") == 0) {
            beacon_timer_.force_cancel();
            if (node_->energy_model()) {
                node_->energy_model()->update_off_time(true);
            }
        }
    }

    if (argc == 3) {
        if (strcasecmp(argv[1], "addr") == 0) {
            my_id_ = Address::instance().str2addr(argv[2]);
            return TCL_OK;
        }

        TclObject *obj;
        if ((obj = TclObject::lookup(argv[2])) == 0) {
            fprintf(stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1], argv[2]);
            return (TCL_ERROR);
        }
        if (strcasecmp(argv[1], "node") == 0) {
            node_ = (MobileNode *) obj;
            return (TCL_OK);
        } else if (strcasecmp(argv[1], "port-dmux") == 0) {
            port_dmux_ = (PortClassifier *) obj; //(NsObject *) obj;
            return (TCL_OK);
        } else if (strcasecmp(argv[1], "tracetarget") == 0) {
            trace_target_ = (Trace *) obj;
            return TCL_OK;
        }
    }// if argc == 3

    return (Agent::command(argc, argv));
}

void EDGRAgent::startUp() {
    this->x_ = node_->X();        // get Location
    this->y_ = node_->Y();
    dest->x_ = node_->destX();
    dest->y_ = node_->destY();

    beacon_timer_.resched(5 + 0.015 * my_id_);

    FILE *fp;
    fp = fopen("Neighbors.tr", "w");
    fclose(fp);

    if (node_->energy_model()) {
        fp = fopen("Energy.tr", "w");
        fclose(fp);
        fp = fopen("EnergyByTime.tr", "w");
        fclose(fp);
    }
}

void EDGRAgent::recv(Packet *p, Handler *h) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_burst *gdh = HDR_BURST(p);
    struct hdr_edgr *edgrh = HDR_EDGR(p);

    switch (cmh->ptype()) {
        case PT_BEACON:
            recvBeacon(p);
            break;

        case PT_CBR:
            /*
             * when a source node receives a CBR packet, it has to send Burst packet or Data packet
             *     + Burst packet is sent first, and one time only
             *     + Then, when the burst packet came back to source node, source node now is able to send Data packet
             */

            if (iph->saddr() == my_id_)         // a packet generated by myself
            {
                if (cmh->num_forwards() == 0)   // a new packet
                {
                    if (is_burst_sent_ == false)
                        sendBurst(p);
                    else if (is_burst_sent_ == true &&
                                (is_burst_left_came_back_ == false || is_burst_right_came_back_ == false))
                        drop(p, "Burst is working");
                    else if (is_burst_left_came_back_ == true && is_burst_right_came_back_ == true)
                        sendData(p);
                    else
                        drop(p, "Unknown node's working mode");
                } else {
                    drop(p, DROP_RTR_ROUTE_LOOP);
                }
            }
            if (iph->ttl_-- <= 0) {
                drop(p, DROP_RTR_TTL);
                return;
            }

            if (gdh->mode_ != 0) {      // burst packet
                if (gdh->direction_ == EDGR_BURST_FORWARD)
                    recvBurst(p, gdh);
                else if (gdh->direction_ == EDGR_BURST_FEED_BACK)
                    recvBurstFeedback(p, gdh);
            } else if (edgrh->dest_addr_ != 0) {        // data packet
                recvData(p, edgrh);
            } else
                drop(p, "Unknown cbr type");

            break;

        default:
            drop(p, "UnknownType");
            break;

    }
}

void EDGRAgent::sendBeacon() {
    if (my_id_ < 0) return;

    Packet *p = allocpkt();
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_beacon *ghh = HDR_BEACON(p);

    cmh->ptype() = PT_BEACON;
    cmh->next_hop_ = IP_BROADCAST;
    cmh->last_hop_ = my_id_;
    cmh->addr_type_ = NS_AF_INET;
    cmh->size() += IP_HDR_LEN + ghh->size();

    iph->daddr() = IP_BROADCAST;
    iph->saddr() = my_id_;
    iph->sport() = RT_PORT;
    iph->dport() = RT_PORT;
    iph->ttl_ = IP_DEF_TTL;

    ghh->location_ = *this;
    ghh->residual_energy_ = node_->energy_model()->energy();

    send(p, 0);
}

void EDGRAgent::recvBeacon(Packet *p) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_beacon *ghh = HDR_BEACON(p);

    if (this->my_id_ == 1173)
        printf("here");
    printf("%f\t%d\t RECV BEACON\n", Scheduler::instance().clock(), this->my_id_);
    addNeighbor(cmh->last_hop_, ghh->location_, ghh->residual_energy_);

    drop(p, "BEACON");
}

void EDGRAgent::sendBurst(Packet *p) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_burst *ghh = HDR_BURST(p);

    cmh->direction_ = hdr_cmn::DOWN;
    cmh->addr_type() = NS_AF_INET;
    cmh->size() += IP_HDR_LEN + ghh->size();


    for (int i = 0; i < 10; i++) {
        ghh->anchor_list_[i] = *dest;
    }
    ghh->dest_ = *dest;
    ghh->anchor_index_ = 0;
    ghh->mode_ = EDGR_BURST_GREEDY;
    ghh->source_ = *this;
    ghh->direction_ = EDGR_BURST_FORWARD;

    iph->saddr() = my_id_;
    iph->daddr() = -1;
    iph->ttl_ = 6 * IP_DEF_TTL;     // max hop-count

    is_burst_sent_ = true;
}

void EDGRAgent::recvBurst(Packet *p, hdr_burst *gdh) {

    // debugging
    printf("%d RECV BURST\n", this->my_id_);

    struct hdr_cmn *cmh = HDR_CMN(p);
    if (cmh->direction() == hdr_cmn::UP && gdh->dest_ == *this) {

        // debugging
        printf("Dest received burst!\n");

        // feed back to source
        struct hdr_ip *iph = HDR_IP(p);
        struct hdr_burst *ghh = HDR_BURST(p);

        cmh->direction_ = hdr_cmn::DOWN;
        cmh->addr_type() = NS_AF_INET;
        cmh->size() += IP_HDR_LEN + ghh->size();

        Point anchors[gdh->anchor_index_];
        for (int i = 0; i < gdh->anchor_index_; i++)
            anchors[i] = gdh->anchor_list_[i];
        for (int i = gdh->anchor_index_ - 1, j = 0; i >= 0; i--, j++) {
            ghh->anchor_list_[j] = anchors[i];
        }
        ghh->anchor_index_ = gdh->anchor_index_;
        ghh->dest_ = gdh->source_;
        ghh->mode_ = EDGR_BURST_GREEDY;
        ghh->source_ = *this;
        ghh->direction_ = EDGR_BURST_FEED_BACK;
        ghh->flag_ = gdh->flag_;

        iph->saddr() = my_id_;
        iph->ttl_ = 6 * IP_DEF_TTL;     // max hop-count

        recvBurstFeedback(p, ghh);
    } else {
        node *nb;

        if (gdh->mode_ == EDGR_BURST_GREEDY) {
            nb = getNeighborByGreedy(gdh->dest_, *this);

            if (nb == NULL) {
                node *nb_left = getNeighborByLeftHandRule(gdh->dest_);
                node *nb_right = getNeighborByRightHandRule(gdh->dest_);

                if (nb_left == NULL) {
                    drop(p, DROP_RTR_NO_ROUTE);
                    return;
                } else {    // shift to bypass mode
                    Packet *p1 = p->copy();

                    // p goes by right hand rule

                    double dis_prev = G::distance(gdh->prev_, G::line(gdh->source_, gdh->dest_));
                    double dis_this = G::distance(*this, G::line(gdh->source_, gdh->dest_));
                    double dis_next = G::distance(*nb_right, G::line(gdh->source_, gdh->dest_));

                    if (dis_this >= dis_prev && dis_this >= dis_next)
                        gdh->anchor_list_[gdh->anchor_index_++] = *this;

                    gdh->mode_ = EDGR_BURST_BYPASS;
                    gdh->void_ = *this;
                    gdh->prev_ = *this;
                    gdh->flag_ = EDGR_BURST_FLAG_RIGHT;


                    cmh->direction_ = hdr_cmn::DOWN;
                    cmh->last_hop_ = my_id_;
                    cmh->next_hop_ = nb_right->id_;

                    send(p, 0);


                    // p1 goes by left hand rule

                    struct hdr_burst *gdh1 = HDR_BURST(p1);

                    dis_prev = G::distance(gdh1->prev_, G::line(gdh1->source_, gdh1->dest_));
                    dis_this = G::distance(*this, G::line(gdh1->source_, gdh1->dest_));
                    dis_next = G::distance(*nb_left, G::line(gdh1->source_, gdh1->dest_));

                    if (dis_this >= dis_prev && dis_this >= dis_next)
                        gdh1->anchor_list_[gdh1->anchor_index_++] = *this;

                    gdh1->mode_ = EDGR_BURST_BYPASS;
                    gdh1->void_ = *this;
                    gdh1->prev_ = *this;
                    gdh1->flag_ = EDGR_BURST_FLAG_LEFT;

                    struct hdr_cmn *cmh1 = HDR_CMN(p1);
                    cmh1->direction_ = hdr_cmn::DOWN;
                    cmh1->last_hop_ = my_id_;
                    cmh1->next_hop_ = nb_left->id_;

                    send(p1, 0);
                }
            } else {
                cmh->direction() = hdr_cmn::DOWN;
                cmh->addr_type() = NS_AF_INET;
                cmh->last_hop_ = my_id_;
                cmh->next_hop_ = nb->id_;
                gdh->prev_ = *this;
                send(p, 0);
            }

        } else if (gdh->mode_ == EDGR_BURST_BYPASS) {
            // try to get back to greedy mode
            nb = getNeighborByGreedy(gdh->dest_, gdh->void_);
            if (nb) {
                gdh->mode_ = EDGR_BURST_GREEDY;
//                gdh->anchor_list_[gdh->anchor_index_++] = *this;
            } else {
                if (gdh->flag_ == EDGR_BURST_FLAG_LEFT) {
                    nb = getNeighborByLeftHandRule(gdh->prev_);
                    count_left_ += 1;
                } else if (gdh->flag_ == EDGR_BURST_FLAG_RIGHT) {
                    nb = getNeighborByRightHandRule(gdh->prev_);
                    count_right_ += 1;
                }

                if (nb == NULL) {
                    drop(p, DROP_RTR_NO_ROUTE);
                    return;
                }

                double dis_prev = G::distance(gdh->prev_, G::line(gdh->source_, gdh->dest_));
                double dis_this = G::distance(*this, G::line(gdh->source_, gdh->dest_));
                double dis_next = G::distance(*nb, G::line(gdh->source_, gdh->dest_));

                if (dis_this >= dis_prev && dis_this >= dis_next)
                    gdh->anchor_list_[gdh->anchor_index_++] = *this;
            }

            gdh->prev_ = *this;

            cmh->direction_ = hdr_cmn::DOWN;
            cmh->last_hop_ = my_id_;
            cmh->next_hop_ = nb->id_;

            send(p, 0);
        } else {
            drop(p, "UnknownType");
            return;
        }
    }
}

void EDGRAgent::recvBurstFeedback(Packet *p, hdr_burst *gdh) {

    printf("%d\tRECV BURST_FEEDBACK\n", this->my_id_);

    struct hdr_cmn *cmh = HDR_CMN(p);
    if (cmh->direction() == hdr_cmn::UP && gdh->dest_ == *this) {

        // print anchor lists
        if (gdh->flag_ == EDGR_BURST_FLAG_LEFT) {
            is_burst_left_came_back_ = true;
            printf("Left: ");
            for (int i = gdh->anchor_index_ - 1, j = 0; i >= 0; i -= 1, j++) {
                printf("%f\t%f\t", gdh->anchor_list_[i].x_, gdh->anchor_list_[i].y_);
                left_anchors_[j] = gdh->anchor_list_[i];
            }
        } else if (gdh->flag_ == EDGR_BURST_FLAG_RIGHT) {
            is_burst_right_came_back_ = true;
            printf("Right: ");
            for (int i = gdh->anchor_index_ - 1, j = 0; i >= 0; i -= 1, j++) {
                printf("%f\t%f\t", gdh->anchor_list_[i].x_, gdh->anchor_list_[i].y_);
                right_anchors_[j] = gdh->anchor_list_[i];
            }
        } else {
            drop(p, "Unknown feedback");
            return;
        }

        printf("\n");
    } else {
        node *nb;

        if (gdh->mode_ == EDGR_BURST_GREEDY) {
            nb = getNeighborByGreedy(gdh->dest_, *this);

            if (nb == NULL) {
                nb = getNeighborByRightHandRule(gdh->dest_);

                if (nb == NULL) {
                    drop(p, DROP_RTR_NO_ROUTE);
                    return;
                } else {    // shift to bypass mode
                    gdh->mode_ = EDGR_BURST_BYPASS;
                    gdh->void_ = *this;
                }
            }

        } else if (gdh->mode_ == EDGR_BURST_BYPASS) {
            // try to get back to greedy mode
            nb = getNeighborByGreedy(gdh->dest_, gdh->void_);
            if (nb) {
                gdh->mode_ = EDGR_BURST_GREEDY;
            } else {
                nb = getNeighborByRightHandRule(gdh->prev_);
                if (nb == NULL) {
                    drop(p, DROP_RTR_NO_ROUTE);
                    return;
                }
            }

        } else {
            drop(p, "UnknownType");
            return;
        }

        gdh->prev_ = *this;
        cmh->direction_ = hdr_cmn::DOWN;
        cmh->last_hop_ = my_id_;
        cmh->next_hop_ = nb->id_;

        send(p, 0);
    }
}

void EDGRAgent::addNeighbor(nsaddr_t nb_id, Point nb_location, float_t nb_energy) {
    neighbor *temp = getNeighbor(nb_id);

    if (temp == NULL)            // it is a new node
    {
        temp = new neighbor;
        temp->id_ = nb_id;
        temp->x_ = nb_location.x_;
        temp->y_ = nb_location.y_;
        temp->residual_energy_ = nb_energy;
        temp->time_ = Scheduler::instance().clock();
        temp->next_ = NULL;

        if (neighbor_list_ == NULL)        // the list now is empty
        {
            neighbor_list_ = temp;
        } else                        // the nodes list is not empty
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
    } else // temp != null
    {
        temp->time_ = NOW;
        temp->x_ = nb_location.x_;
        temp->y_ = nb_location.y_;
        temp->residual_energy_ = nb_energy;
    }
}

neighbor *EDGRAgent::getNeighbor(nsaddr_t nid) {
    for (neighbor *temp = neighbor_list_; temp; temp = (neighbor *) temp->next_) {
        if (temp->id_ == nid) return temp;
    }
    return NULL;
}

neighbor *EDGRAgent::getNeighbor(Point p) {
    for (neighbor *temp = neighbor_list_; temp; temp = (neighbor *) temp->next_) {
        if (temp->x_ == p.x_ && temp->y_ == p.y_)
            return temp;
    }
    return NULL;
}

neighbor *EDGRAgent::getNeighborByGreedy(Point d, Point s) {
    //initializing the minimal distance as my distance to sink
    double mindis = G::distance(s, d);
    neighbor *re = NULL;

    for (node *temp = neighbor_list_; temp; temp = temp->next_) {
        double dis = G::distance(temp, d);
        if (dis < mindis) {
            mindis = dis;
            re = (neighbor *) temp;
        }
    }
    return re;
}

neighbor *EDGRAgent::getNeighborByRightHandRule(Point p) {
    Angle max_angle = -1;
    neighbor *nb = NULL;

    for (node *temp = neighbor_list_; temp; temp = temp->next_) {
        {
            Angle a = G::angle(this, &p, this, temp);
            if (a > max_angle) {
                max_angle = a;
                nb = (neighbor *) temp;
            }
        }
    }

    return nb;
}

neighbor *EDGRAgent::getNeighborByLeftHandRule(Point p) {
    Angle min_angle = DBL_MAX;
    neighbor *nb = NULL;

    for (node *temp = neighbor_list_; temp; temp = temp->next_) {
        {
            Angle a = G::angle(this, &p, this, temp);
            if (a < min_angle && a != 0) {
                min_angle = a;
                nb = (neighbor *) temp;
            }
        }
    }

    return nb;
}

void EDGRAgent::sendData(Packet *p) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_edgr *ghh = HDR_EDGR(p);

    cmh->direction_ = hdr_cmn::DOWN;
    cmh->addr_type() = NS_AF_INET;
    cmh->size() += IP_HDR_LEN + ghh->size();

    ghh->dest_addr_ = iph->daddr();

    for (int i = 0; i < 10; i++) {
        ghh->anchor_list_[i] = *dest;
    }

    // choose a random anchor list
    srand(time(0));
    int ra_ = rand() % 2;
    if (ra_ == 0) {
        for (int i = 0; left_anchors_[i].x_ != 0 && left_anchors_[i].y_ != 0; i++) {
            ghh->anchor_list_[i] = left_anchors_[i];
        }
    } else {
        for (int i = 0; right_anchors_[i].x_ != 0 && right_anchors_[i].y_ != 0; i++) {
            ghh->anchor_list_[i] = right_anchors_[i];
        }
    }

    iph->saddr() = my_id_;
    iph->daddr() = -1;
    iph->ttl_ = 6 * IP_DEF_TTL;     // max hop-count
}

void EDGRAgent::recvData(Packet *p, hdr_edgr *gdh) {

    printf("%f\tRECV DATA\n", this->my_id_);

    struct hdr_cmn *cmh = HDR_CMN(p);

    neighbor *nb;
    if (cmh->direction() == hdr_cmn::UP && gdh->dest_addr_ == my_id_) {
        port_dmux_->recv(p, 0);
    } else {
        Point sub_dest_ = gdh->anchor_list_[0];
        if (sub_dest_ == *this) {       // reached a sub-destination
            // --> remove it from anchor list and derive the next sub destination
            for (int i = 0; i < 19; i++) {
                gdh->anchor_list_[i] = gdh->anchor_list_[i + 1];
            }
            sub_dest_ = gdh->anchor_list_[0];
        }

        if (G::distance(*this, sub_dest_) <= d_o_) {
            // d_o_ is always smaller than transmission range
            // --> find in the neighbor list which node have coordinate same as sub_dest
            nb = getNeighbor(sub_dest_);
        } else {
            nb = findOptimizedForwarder(sub_dest_);
        }

        cmh->direction_ = hdr_cmn::DOWN;
        cmh->last_hop_ = my_id_;
        cmh->next_hop_ = nb->id_;

        send(p, 0);
    }
}

neighbor *EDGRAgent::findOptimizedForwarder(Point p) {
    Point ideal_relay_;
    double d_u_a1_ = G::distance(*this, p);
    ideal_relay_.x_ = this->x_ + d_opt_ * (p.x_ - this->x_) / d_u_a1_;
    ideal_relay_.y_ = this->y_ + d_opt_ * (p.y_ - this->y_) / d_u_a1_;
    float_t relay_region_radius = 4 / 5 * d_u_a1_;

    double max_ = 0;
    neighbor *re_ = NULL;
    for (node *temp = neighbor_list_; temp; temp = temp->next_) {
        if (G::distance(temp, p) < relay_region_radius) {
            neighbor* nb = getNeighbor(*temp);
            double base = gamma_ * nb->residual_energy_ + (1 - gamma_) * G::distance(nb, ideal_relay_);
            if (base > max_) {
                max_ = base;
                re_ = nb;
            }
        }
    }

    return re_;
}

void EDGRAgent::dumpEnergy() {
    if (node_->energy_model()) {
        FILE *fp = fopen("Energy.tr", "a+");
        fprintf(fp, "%d\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", my_id_, this->x_, this->y_,
                node_->energy_model()->energy(),
                node_->energy_model()->off_time(),
                node_->energy_model()->et(),
                node_->energy_model()->er(),
                node_->energy_model()->ei(),
                node_->energy_model()->es()
        );
        fclose(fp);
    }
}

void EDGRAgent::dumpNeighbor() {
    FILE *fp = fopen("Neighbors.tr", "a+");
    fprintf(fp, "%d	%f	%f	%f	", this->my_id_, this->x_, this->y_, node_->energy_model()->off_time());
    for (node *temp = neighbor_list_; temp; temp = temp->next_) {
        fprintf(fp, "%d,", temp->id_);
    }
    fprintf(fp, "\n");

    fclose(fp);
}

void EDGRAgent::dumpEnergyByTime() {
    if (node_->energy_model()) {
        FILE *fp = fopen("EnergyByTime.tr", "a+");
        fprintf(fp, "%f\t%d\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", NOW, my_id_, this->x_, this->y_,
                node_->energy_model()->energy(),
                node_->energy_model()->et(),
                node_->energy_model()->er(),
                node_->energy_model()->ei(),
                node_->energy_model()->es()
        );
        fclose(fp);
    }
}
