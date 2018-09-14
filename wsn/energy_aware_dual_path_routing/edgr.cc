#include <trace/cmu-trace.h>
#include "edgr.h"

int hdr_beacon::offset_;
int hdr_burst::offset_;

/*
 * BEACON Header Class
 */
static class BeaconHeaderClass : public PacketHeaderClass {
public:
    BeaconHeaderClass() : PacketHeaderClass("PacketHeader/BEACON", sizeof(hdr_beacon)) {
        bind_offset(&hdr_beacon::offset_);
    }

    ~BeaconHeaderClass() {}
} class_beaconhdr;


/*
 * BURST Header Class
 */
static class BurstHeaderClass : public PacketHeaderClass {
public:
    BurstHeaderClass() : PacketHeaderClass("PacketHeader/BURST", sizeof(hdr_burst)) {
        bind_offset(&hdr_burst::offset_);
    }

    ~BurstHeaderClass() {}
};

/*
 * Timer
 */
void AgentBroadcastTimer::expire(Event *e) {
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

    switch (cmh->ptype()) {
        case PT_BEACON:
            recvBeacon(p);
            break;

        case PT_CBR:
            if (HDR_BURST(p)) {
                sendBurst(p);
                recvBurst(p);
            }
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

    addNeighbor(cmh->last_hop_, ghh->location_, ghh->residual_energy_);

    drop(p, "BEACON");
}

void EDGRAgent::sendBurst(Packet *p) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_burst *ghh = HDR_BURST(p);

    cmh->last_hop_ = my_id_;
    cmh->addr_type_ = NS_AF_INET;
    cmh->size() += IP_HDR_LEN + ghh->size();

    iph->saddr() = my_id_;
    iph->sport() = RT_PORT;
    iph->dport() = RT_PORT;
    iph->ttl_ = IP_DEF_TTL;

    for (int i = 0; i < 20; i++) {
        ghh->anchor_list_[i] = *dest;
    }
    ghh->dest_ = *dest;
    ghh->anchor_index_ = 0;
    ghh->mode_ = EDGR_BURST_GREEDY;
}

void EDGRAgent::recvBurst(Packet *p) {
    struct hdr_burst *gdh = HDR_BURST(p);
    recvBurst(p, gdh);
}

void EDGRAgent::recvBurst(Packet *p, hdr_burst *gdh) {
    struct hdr_cmn *cmh = HDR_CMN(p);

    if (cmh->direction() == hdr_cmn::UP && gdh->daddr_ == my_id_) {
        Point *anchor_list = gdh->anchor_list_;

        // print 2 anchor lists
    } else {
        node *nb;

        switch (gdh->mode_) {
            case EDGR_BURST_GREEDY:
                nb = getNeighborByGreedy(gdh->dest_, *this);

                if (nb == NULL) {
                    node *nb_left = getNeighborByLeftHandRule(gdh->dest_);
                    node *nb_right = getNeighborByLeftHandRule(gdh->dest_);

                    if (nb_left == NULL) {
                        drop(p, DROP_RTR_NO_ROUTE);
                        return;
                    } else {
                        Packet *right_hand_pkt = allocpkt();
                        Packet *left_hand_pkt = allocpkt();

                        // add info to right_hand_pkt
                        struct hdr_cmn *cmh1 = HDR_CMN(right_hand_pkt);
                        struct hdr_ip *iph1 = HDR_IP(right_hand_pkt);
                        struct hdr_burst *ghh1 = HDR_BURST(right_hand_pkt);

                        cmh->last_hop_ = my_id_;
                        cmh->addr_type_ = NS_AF_INET;
                        cmh->size() += IP_HDR_LEN + ghh1->size();

                        iph1->saddr() = my_id_;
                        iph1->sport() = RT_PORT;
                        iph1->dport() = RT_PORT;
                        iph1->ttl_ = IP_DEF_TTL;

                        for (int i = 0; i < 20; i++) {
                            ghh1->anchor_list_[i] = *dest;
                        }
                        ghh1->dest_ = gdh->dest_;
                        ghh1->anchor_index_ = 0;
                        ghh1->mode_ = EDGR_BURST_BYPASS;
                        ghh1->anchor_list_[ghh1->anchor_index_] = *nb_right;
                        ghh1->anchor_index_ += 1;
                        ghh1->flag_ = EDGR_BURST_FLAG_RIGHT;
                        ghh1->void_ = *this;
                        ghh1->prev_ = *this;


                        // add info to left hand pkt
                        struct hdr_cmn *cmh2 = HDR_CMN(left_hand_pkt);
                        struct hdr_ip *iph2 = HDR_IP(left_hand_pkt);
                        struct hdr_burst *ghh2 = HDR_BURST(left_hand_pkt);

                        cmh->last_hop_ = my_id_;
                        cmh->addr_type_ = NS_AF_INET;
                        cmh->size() += IP_HDR_LEN + ghh1->size();

                        iph2->saddr() = my_id_;
                        iph2->sport() = RT_PORT;
                        iph2->dport() = RT_PORT;
                        iph2->ttl_ = IP_DEF_TTL;

                        for (int i = 0; i < 20; i++) {
                            ghh1->anchor_list_[i] = *dest;
                        }
                        ghh2->dest_ = gdh->dest_;
                        ghh2->anchor_index_ = 0;
                        ghh2->mode_ = EDGR_BURST_BYPASS;
                        ghh2->anchor_list_[ghh1->anchor_index_] = *nb_left;
                        ghh2->anchor_index_ += 1;
                        ghh2->flag_ = EDGR_BURST_FLAG_LEFT;
                        ghh2->void_ = *this;
                        ghh2->prev_ = *this;


                        cmh1->direction_ = hdr_cmn::DOWN;
                        cmh1->last_hop_ = my_id_;
                        cmh1->next_hop_ = nb->id_;

                        cmh2->direction_ = hdr_cmn::DOWN;
                        cmh2->last_hop_ = my_id_;
                        cmh2->next_hop_ = nb->id_;

                        send(right_hand_pkt, 0);
                        send(left_hand_pkt, 0);
                    }
                }
                break;

            case EDGR_BURST_BYPASS:
                // try to get back to greedy mode
                nb = getNeighborByGreedy(gdh->dest_, gdh->void_);
                if (nb) {
                    gdh->mode_ = EDGR_BURST_GREEDY;
                } else {
                    if (gdh->flag_ == EDGR_BURST_FLAG_LEFT) {
                        nb = getNeighborByLeftHandRule(gdh->prev_);
                        if (nb == NULL) {
                            drop(p, DROP_RTR_NO_ROUTE);
                            return;
                        }
                    }
                    else if (gdh->flag_ == EDGR_BURST_FLAG_RIGHT) {
                        nb = getNeighborByRightHandRule(gdh->prev_);
                        if (nb == NULL) {
                            drop(p, DROP_RTR_NO_ROUTE);
                            return;
                        }
                    }
                }
                break;

            default:
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
        //if (temp->planar_)
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
        //if (temp->planar_)
        {
            Angle a = G::angle(this, &p, this, temp);
            if (a < min_angle) {
                min_angle = a;
                nb = (neighbor *) temp;
            }
        }
    }

    return nb;
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
