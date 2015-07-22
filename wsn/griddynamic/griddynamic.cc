#include "config.h"

#include "griddynamic.h"
#include "griddynamic_packet.h"

#include "wsn/gridoffline/gridoffline_packet_data.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int hdr_griddynamic::offset_;
node *GridDynamicAgent::sink_list_ = NULL;

static class GridDynamicHeaderClass : public PacketHeaderClass {
public:
    GridDynamicHeaderClass() : PacketHeaderClass("PacketHeader/GRIDDYNAMIC", sizeof(hdr_griddynamic)) {
        bind_offset(&hdr_griddynamic::offset_);
    }
} class_griddynamichdr;

static class GridDynamicAgentClass : public TclClass {
public:
    GridDynamicAgentClass() : TclClass("Agent/GRIDDYNAMIC") { }

    TclObject *create(int, const char *const *) {
        return (new GridDynamicAgent());
    }
} class_grid;

void
GridDynamicTimer::expire(Event *e) {
    (a_->*firing_)();
}

// ------------------------ Agent ------------------------ //

GridDynamicAgent::GridDynamicAgent() : GPSRAgent(),
                                       findStuck_timer_(this, &GridDynamicAgent::findStuckAngle),
                                       updateSta_timer_(this, &GridDynamicAgent::updateState) {
    isBoundary = false;
    isSink = false;
    isStuck = false;
    isAlerted = false;
    isSinkNotify = false;
    forceFindStuck = true;
    pivot.id_ = -1;
    stuck_angle_ = NULL;
    hole_ = NULL;
    tracking_list_ = NULL;
    max_node_list_ = NULL;
    bind("range_", &range_);
    bind("limit_", &limit_);
    bind("r_", &r_);
    bind("limit_boundhole_hop_", &limit_boundhole_hop_);
    bind("limit_x_", &limit_x_);
    bind("limit_y_", &limit_y_);
    bind("alert_threshold_", &alert_threshold_);
    bind("nodeoff_threshold_", &nodeoff_threshold_);

    nodeoff_threshold_ = hello_period_*nodeoff_threshold_;
}

int
GridDynamicAgent::command(int argc, const char *const *argv) {
    if (argc == 2) {
        if (strcasecmp(argv[1], "start") == 0) {
            startUp();
        }
        if (strcasecmp(argv[1], "nodesink") == 0) {
            findStuck_timer_.force_cancel();
            updateSta_timer_.force_cancel();
            // add sink
            isSink = true;
            node *temp = new node();
            temp->next_ = sink_list_;
            temp->x_ = this->x_;
            temp->y_ = this->y_;
            sink_list_ = temp;
        }
        if (strcasecmp(argv[1], "nodeoff") == 0) {
            if (node_->energy_model()->off_time() <= 0)
                dumpNodeOffReal();
            findStuck_timer_.force_cancel();
            updateSta_timer_.force_cancel();
        }
    }

    int re = GPSRAgent::command(argc, argv);
    nx_ = (int) x_ / r_;
    ny_ = (int) y_ / r_;

    return re;
}

// handle the receive packet just of type PT_GRID
void
GridDynamicAgent::recv(Packet *p, Handler *h) {
    hdr_cmn *cmh = HDR_CMN(p);

    switch (cmh->ptype()) {
        case PT_HELLO:
            GPSRAgent::recv(p, h);
            break;

        case PT_GRID:
            recvBoundHole(p);
            break;

        case PT_GRIDDYNAMIC:
            recvGridDynamic(p);
            break;

        case PT_CBR:
            drop(p, " Unhandle");
            break;

        default:
            drop(p, " UnknowType");
            break;
    }
}

void
GridDynamicAgent::startUp() {
    // printf("%f - startUp\n", NOW);
    //dumpHoleArea();

    pivot.id_ = -1;

    if (hello_period_ != 0) {
        findStuck_timer_.resched(hello_period_ * 1.5);
    } else {
        printf("Warning: %s\n", "hello_period_ must be not equal to zero");
        findStuck_timer_.force_cancel();
    }

    // clear trace file
    FILE *fp;
    fp = fopen("Election.tr", "w");
    fclose(fp);
    fp = fopen("Pivot.tr", "w");
    fclose(fp);
    fp = fopen("Alarm.tr", "w");
    fclose(fp);
    fp = fopen("PolygonHole.tr", "w");
    fclose(fp);
    fp = fopen("Neighbors_2.tr", "w");
    fclose(fp);
    fp = fopen("NodeOff.tr", "w");
    fclose(fp);
    fp = fopen("NodeOffReal.tr", "w");
    fclose(fp);
}

void
GridDynamicAgent::recvGridDynamic(Packet *p) {
    hdr_griddynamic *hgd = HDR_GRIDDYNAMIC(p);
    switch (hgd->type_) {
        case GRID_UPDATE:
            recvUpdate(p);
            break;
        case GRID_PIVOT:
            recvPivot(p);
            break;
        case GRID_NOTIFY:
            recvNotify(p);
            break;
        case GRID_COLLECT:
            recvCollect(p);
            break;
    }
}

// region ------------------------ Bound hole ------------------------ //

void
GridDynamicAgent::findStuckAngle() {
    findStuck_timer_.resched(hello_period_);
    // remove nodeoff
    if (!removeNodeoff()) {
        return;
    }
    
    forceFindStuck = false;

    // update stuck node state
    if (neighbor_list_ == NULL || neighbor_list_->next_ == NULL) {
        stuck_angle_ = NULL;
        return;
    }

    node *nb1 = neighbor_list_;
    node *nb2 = neighbor_list_->next_;

    while (nb2) {
        Circle circle = G::circumcenter(this, nb1, nb2);
        Angle a = G::angle(this, nb1, this, &circle);
        Angle b = G::angle(this, nb1, this, nb2);
        Angle c = G::angle(this, &circle, this, nb2);

        // if O is outside range of node, nb1 and nb2 create a stuck angle with node
        if (b >= M_PI || (fabs(a) + fabs(c) == fabs(b) && G::distance(this, circle) > range_)) {
            stuckangle *new_angle = new stuckangle();
            new_angle->a_ = nb1;
            new_angle->b_ = nb2;
            new_angle->next_ = stuck_angle_;
            stuck_angle_ = new_angle;
        }

        nb1 = nb1->next_;
        nb2 = nb1->next_;
    }

    nb2 = neighbor_list_;
    Circle circle = G::circumcenter(this, nb1, nb2);
    Angle a = G::angle(this, nb1, this, &circle);
    Angle b = G::angle(this, nb1, this, nb2);
    Angle c = G::angle(this, &circle, this, nb2);

    // if O is outside range of node, nb1 and nb2 create a stuck angle with node
    if (b >= M_PI || (fabs(a) + fabs(c) == fabs(b) && G::distance(this, circle) > range_)) {
        stuckangle *new_angle = new stuckangle();
        new_angle->a_ = nb1;
        new_angle->b_ = nb2;
        new_angle->next_ = stuck_angle_;
        stuck_angle_ = new_angle;
    }

    if (stuck_angle_) {
        sendBoundHole();
    }
}

void
GridDynamicAgent::sendBoundHole() {
    Packet *p;
    hdr_cmn *cmh;
    hdr_ip *iph;
    hdr_grid *gh;
    if (my_id_ == 496) {
        int i = 0;
    }

    isStuck = true;
    isBoundary = true;
    findStuck_timer_.force_cancel();

    for (stuckangle *sa = stuck_angle_; sa; sa = sa->next_) {
        p = allocpkt();

        p->setdata(new GridOfflinePacketData());

        cmh = HDR_CMN(p);
        iph = HDR_IP(p);
        gh = HDR_GRID(p);

        cmh->ptype() = PT_GRID;
        cmh->direction() = hdr_cmn::DOWN;
        cmh->size() += IP_HDR_LEN + gh->size();
        cmh->next_hop_ = sa->a_->id_;
        cmh->last_hop_ = my_id_;
        cmh->addr_type_ = NS_AF_INET;

        iph->saddr() = my_id_;
        iph->daddr() = sa->a_->id_;
        iph->sport() = RT_PORT;
        iph->dport() = RT_PORT;
        iph->ttl_ = limit_boundhole_hop_;            // more than ttl_ hop => boundary => remove

        gh->prev_ = *this;
        gh->last_ = *(sa->b_);
        gh->i_ = *this;

        send(p, 0);

        //printf("%d\t- Send GridDynamic (%f)\n", my_id_, NOW);
    }

    // send Pivot
    sendPivot();
}

void
GridDynamicAgent::recvBoundHole(Packet *p) {
    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_grid *gh = HDR_GRID(p);

    isBoundary = true;

    // add data to packet
    addData(p);

    if (isStuck) {
        sendPivot();
        //printf("Send collect from 2 BoundHole:%d (%d) %f-%f at %f\n", iph->src_, my_id_, this->x_, this->y_, NOW);
        sendCollect(p);

        drop(p, "reach stuck node");
        return;
    }

    // if the grid packet has came back to the initial node
    if (iph->saddr() == my_id_) {
        createGridHole(p, *this);
        hole_->hole_id_ = my_id_;
        hole_->dump();

        dumpBoundhole(hole_);

        sendPivot();
        //printf("Send collect from 1 BoundHole:%d (%d) %f-%f\n", iph->src_, my_id_, this->x_, this->y_);
        sendCollect(p);

        drop(p, "reach stuck node");
        return;
    }

    // check to election
    if (!checkCell(gh->prev_)) {
        sendPivot();
    }

    // forward this packet
    if (iph->ttl_-- <= 0) {
        drop(p, DROP_RTR_TTL);
        return;
    }

    node *nb = getNeighborByBoundhole(&gh->prev_, &gh->last_);

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

    gh->last_ = gh->prev_;
    gh->prev_ = *this;

    send(p, 0);
}

// ------------------------ Broadcast pivot -----------------------//

void
GridDynamicAgent::sendNotify() {
    dumpPivot();

    //schedule for check state
    updateSta_timer_.resched(hello_period_ * 1.5);

    pivot.id_ = my_id_;
    pivot.x_ = this->x_;
    pivot.y_ = this->y_;

    Packet *p = allocpkt();
    hdr_cmn *cmh = HDR_CMN(p);
    hdr_ip *iph = HDR_IP(p);
    hdr_griddynamic *gdh = HDR_GRIDDYNAMIC(p);

    cmh->ptype() = PT_GRIDDYNAMIC;
    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = IP_BROADCAST;
    cmh->last_hop_ = my_id_;
    cmh->addr_type_ = NS_AF_INET;

    iph->daddr() = IP_BROADCAST;
    iph->sport() = RT_PORT;
    iph->dport() = RT_PORT;

    gdh->id_ = pivot.id_;
    gdh->p_.x_ = pivot.x_;
    gdh->p_.y_ = pivot.y_;
    gdh->type_ = GRID_NOTIFY;

    send(p, 0);
}

void
GridDynamicAgent::recvNotify(Packet *p) {
    hdr_cmn *cmh = HDR_CMN(p);
    hdr_griddynamic *gdh = HDR_GRIDDYNAMIC(p);

    if (checkCell(gdh->p_)) {
        // update pivot in case same pivot
        if (pivot.id_ >= 0 && pivot.id_ <= gdh->id_) {
            drop(p, "repeated notify");
            return;
        }

        pivot.id_ = gdh->id_;
        pivot.x_ = gdh->p_.x_;
        pivot.y_ = gdh->p_.y_;

        // replace hello packet by updateState packet
        // hello_timer_.force_cancel();
        findStuck_timer_.force_cancel();
        updateSta_timer_.resched(randSend_.uniform(0.0, 5));

        // continue forwarding
        cmh->direction() = hdr_cmn::DOWN;
        cmh->next_hop_ = IP_BROADCAST;
        cmh->last_hop_ = my_id_;
        cmh->addr_type_ = NS_AF_INET;

        send(p, 0);
    } else {
        drop(p, "different cell");
    }
}

// ------------------------ Dynamic process --------------------------- //

void GridDynamicAgent::updateState() {
    removeNodeoff();
    if (my_id_ == pivot.id_) {
        checkState();
    } else {
        sendUpdate();
    }

    updateSta_timer_.resched(hello_period_);
}

void
GridDynamicAgent::checkState() {
    if (removeTrackingNodeOff()) {
        int count = 0, count_max_ = 0;
        for (node *temp = tracking_list_; temp; temp = temp->next_) count++;
        for (node *temp = max_node_list_; temp; temp = temp->next_) count_max_++;
        printf("[check state] %d(%d-%d) count: %d, countmax: %d at %f\n", my_id_, nx_, ny_, count, count_max_, NOW);
        if (!isAlerted) {
            if ((100 - alert_threshold_) / 100.0 > 1.0 * count / count_max_) {
                printf("alert-%d/%d\n", count, count_max_);
                sendAlarm();
            }
        }
        if (!isSinkNotify) {
            if ((100 - sink_threshold_) / 100.0 > 1.0 * count / count_max_) {
                printf("sink-%d/%d\n", count, count_max_);
                Packet *p = allocpkt();

                GridOfflinePacketData *data = new GridOfflinePacketData();
                p->setdata(data);
                sendCollect(p);
            }
        }
    }
}

void
GridDynamicAgent::sendPivot() {
    if (pivot.id_ >= 0) {
        return;
    }

    //dumpElection();

    Point dest;
    dest.x_ = r_ * (nx_ + 0.5);
    dest.y_ = r_ * (ny_ + 0.5);

    node *next = getNeighborByGreedy(dest);

    // forward to dest
    if (next != NULL) {
        Packet *p = allocpkt();
        hdr_cmn *cmh = HDR_CMN(p);
        hdr_ip *iph = HDR_IP(p);
        hdr_griddynamic *gdh = HDR_GRIDDYNAMIC(p);

        cmh->ptype() = PT_GRIDDYNAMIC;
        cmh->direction() = hdr_cmn::DOWN;
        cmh->size() += IP_HDR_LEN + gdh->size();
        cmh->next_hop_ = next->id_;
        cmh->last_hop_ = my_id_;
        cmh->addr_type_ = NS_AF_INET;

        iph->saddr() = my_id_;
        iph->daddr() = next->id_;
        iph->sport() = RT_PORT;
        iph->dport() = RT_PORT;

        gdh->type_ = GRID_PIVOT;
        gdh->p_ = dest;

        send(p, 0);
    } else {
        sendNotify();
        if (isBoundary) {
            sendAlarm();

            Packet *p = allocpkt();
            GridOfflinePacketData *data = new GridOfflinePacketData();
            p->setdata(data);
            sendCollect(p);
        }
    }
}

void
GridDynamicAgent::sendAlarm() {
    dumpAlarm();
    if (my_id_ == 1066){
        int i = 0;
    }
    isAlerted = true;
    Point *dests = new Point[4];
    Point up, down, left, right;
    int cell_x_ = r_ * (nx_ + 0.5);
    int cell_y_ = r_ * (ny_ + 0.5);

    up.x_ = down.x_ = cell_x_;
    up.y_ = cell_y_ + r_;
    down.y_ = cell_y_ - r_;

    left.y_ = right.y_ = cell_y_;
    left.x_ = cell_x_ - r_;
    right.x_ = cell_x_ + r_;

    dests[0] = up;
    dests[1] = down;
    dests[2] = left;
    dests[3] = right;


    for (int i = 0; i < 4; i++) {
        node *next = getNeighborByGreedy(dests[i]);

        if (next != NULL) {
            Packet *p = allocpkt();
            hdr_cmn *cmh = HDR_CMN(p);
            hdr_ip *iph = HDR_IP(p);
            hdr_griddynamic *gdh = HDR_GRIDDYNAMIC(p);

            cmh->ptype() = PT_GRIDDYNAMIC;
            cmh->direction() = hdr_cmn::DOWN;
            cmh->size() += IP_HDR_LEN + gdh->size();
            cmh->next_hop_ = next->id_;
            cmh->last_hop_ = my_id_;
            cmh->addr_type_ = NS_AF_INET;

            iph->saddr() = my_id_;
            iph->sport() = RT_PORT;
            iph->dport() = RT_PORT;

            gdh->type_ = GRID_PIVOT;
            gdh->p_ = dests[i];

            send(p, 0);
        }
    }

    //printf("Send collect from ALARM (%d) %f-%f at %f\n", my_id_, x_, y_, NOW);
}

void
GridDynamicAgent::recvPivot(Packet *p) {
    hdr_cmn *cmh = HDR_CMN(p);
    hdr_griddynamic *gdh = HDR_GRIDDYNAMIC(p);
    node *next = NULL;

    if (HDR_IP(p)->saddr() == 1066){
        int i = 0;
    }

    if (!checkCell(gdh->p_)) {
        // forward to neighbor cell
        next = getNeighborByGreedy(gdh->p_);

        if (next == NULL) {
            drop(p, DROP_RTR_NO_ROUTE);
            return;
        } else {
            // keep forward
        }
    } else {
        if (pivot.id_ >= 0) {
            drop(p, "known pivot");
            return;
        }

        next = getNeighborByGreedy(gdh->p_);

        if (next == NULL) { // no node is closer than itselt
            drop(p);
            sendNotify();
            return;
        } else {
            // keep forward
        }
    }


    /* foward */

    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = next->id_;
    cmh->last_hop_ = my_id_;
    cmh->addr_type_ = NS_AF_INET;

    send(p, 0);
}

// --------------------- Update state ------------------------------ //
void GridDynamicAgent::sendUpdate() {

    node *next = getNeighborByGreedy(pivot);
    //printf("Elemenet send update(%d-%d): %d,%d\n", pivot.id_, my_id_, nx_, ny_);

    // forward to dest
    if (next != NULL) {
        Packet *p = allocpkt();
        hdr_cmn *cmh = HDR_CMN(p);
        hdr_ip *iph = HDR_IP(p);
        hdr_griddynamic *gdh = HDR_GRIDDYNAMIC(p);

        cmh->ptype() = PT_GRIDDYNAMIC;
        cmh->direction() = hdr_cmn::DOWN;
        cmh->size() += IP_HDR_LEN + gdh->size();
        cmh->next_hop_ = next->id_;
        cmh->last_hop_ = my_id_;
        cmh->addr_type_ = NS_AF_INET;

        iph->saddr() = my_id_;
        iph->daddr() = pivot.id_;
        iph->sport() = RT_PORT;
        iph->dport() = RT_PORT;

        gdh->type_ = GRID_UPDATE;
        gdh->p_ = *this;
        gdh->id_ = this->my_id_;

        send(p, 0);
    }
}

void GridDynamicAgent::recvUpdate(Packet *p) {
    hdr_ip *iph = HDR_IP(p);
    hdr_cmn *cmh = HDR_CMN(p);
    hdr_griddynamic *gdh = HDR_GRIDDYNAMIC(p);

    if (iph->daddr() == my_id_) {
        addTracking(iph->saddr(), gdh->p_);
        // update max neighbor
        for (neighbor *temp = tracking_list_; temp; temp = (neighbor *) temp->next_) {
            node *prev = NULL;
            bool check = false;
            for (node *n = max_node_list_; n; n = n->next_) {
                if (n->id_ > temp->id_) {
                    prev = n;
                    continue;
                }
                if (n->id_ == temp->id_) {
                    check = true;
                }
                break;
            }
            if (!check) {
                if (prev == NULL) {
                    node *n = new node();
                    n->id_ = temp->id_;
                    n->next_ = max_node_list_;
                    max_node_list_ = n;
                } else {
                    node *n = new node();
                    n->id_ = temp->id_;
                    n->next_ = prev->next_;
                    prev->next_ = n;
                }
            }
        }

    } else {
        if (pivot.id_ == -1) {
            drop(p, "dont have pivot");
            return;
        }
        // forward
        node *next = getNeighborByGreedy(pivot);

        if (next != NULL) {
            cmh->direction() = hdr_cmn::DOWN;
            cmh->next_hop_ = next->id_;
            cmh->last_hop_ = my_id_;
            cmh->addr_type_ = NS_AF_INET;

            send(p, 0);
        }
    }
}

/* --------------------- Send data -------------------------------- */
void GridDynamicAgent::sendCollect(Packet *p) {
    isSinkNotify = true;
    dumpCollect();
    drop(p);
}


void GridDynamicAgent::recvCollect(Packet *p) {
}

/* --------------------- HELPER FUNCTIONS ------------------------- */

node *GridDynamicAgent::getNeighborByBoundhole(Point *p, Point *prev) {
    Angle max_angle = -1;
    node *nb = NULL;

    for (node *temp = neighbor_list_; temp; temp = temp->next_) {
        Angle a = G::angle(this, p, this, temp);
        if (a > max_angle && (!G::is_intersect(this, temp, p, prev) ||
                              (temp->x_ == p->x_ && temp->y_ == p->y_) ||
                              (this->x_ == prev->x_ && this->y_ == prev->y_))) {
            max_angle = a;
            nb = temp;
        }
    }

    return nb;
}

/* remove nodeoff in neighbor_list_
 * return true if nodeof is detected
 */
bool GridDynamicAgent::removeNodeoff() {
    // update
    neighbor *next = NULL, *prev = NULL;
    bool isChange = false;

    // remove ahead
    for (neighbor *temp = neighbor_list_; temp; temp = next) {
        next = (neighbor *) temp->next_;
        if (NOW - temp->time_ > nodeoff_threshold_) {
            isChange = true;
            neighbor_list_ = next;
            dumpNodeOff(temp);
            free(temp);
        } else break;
    }

    //remove mid
    for (neighbor *temp = neighbor_list_; temp; temp = next) {
        next = (neighbor *) temp->next_;
        if (NOW - temp->time_ > nodeoff_threshold_) {
            isChange = true;
            prev->next_ = next;
            dumpNodeOff(temp);
            free(temp);
        } else {
            prev = temp;
        }
    }

    return isChange;
}

bool GridDynamicAgent::removeTrackingNodeOff(){
    neighbor *next = NULL, *prev = NULL;
    bool isChange = false;

    // remove ahead
    for (neighbor *temp = tracking_list_; temp; temp = next) {
        next = (neighbor *) temp->next_;
        if (NOW - temp->time_ > nodeoff_threshold_) {
            isChange = true;
            tracking_list_ = next;
            free(temp);
        } else break;
    }

    //remove mid
    for (neighbor *temp = tracking_list_; temp; temp = next) {
        next = (neighbor *) temp->next_;
        if (NOW - temp->time_ > nodeoff_threshold_) {
            isChange = true;
            prev->next_ = next;
            free(temp);
        } else {
            prev = temp;
        }
    }

    return isChange;
}

/* return true if point is in same cell with this node */
bool
GridDynamicAgent::checkCell(Point point) {
    return ((int) (point.x_ / r_) == nx_ && (int) (point.y_ / r_) == ny_);
}

void
GridDynamicAgent::addData(Packet *p) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_grid *bhh = HDR_GRID(p);
    GridOfflinePacketData *data = (GridOfflinePacketData *) p->userdata();

    // Add data to packet
    if (cmh->num_forwards_ == 1) {
        if (fmod(bhh->i_.x_, r_) == 0)    // i lies in vertical line
        {
            if (this->x_ > bhh->i_.x_) bhh->i_.x_ += r_ / 2;
            else if (this->x_ < bhh->i_.x_) bhh->i_.x_ -= r_ / 2;
            else // (this->x_ == bhh->i_.x_)
            {
                if (this->y_ > bhh->i_.y_) bhh->i_.x_ += r_ / 2;
                else bhh->i_.x_ -= r_ / 2;
            }
        }
        if (fmod(bhh->i_.y_, r_) == 0)    // i lies in h line
        {
            if (this->y_ > bhh->i_.y_) bhh->i_.y_ += r_ / 2;
            else if (this->y_ < bhh->i_.y_) bhh->i_.y_ -= r_ / 2;
            else // (this->y_ == bhh->i_.y_)
            {
                if (this->x_ > bhh->i_.x_) bhh->i_.y_ -= r_ / 2;
                else bhh->i_.y_ += r_ / 2;
            }
        }

        bhh->i_.x_ = ((int) (bhh->i_.x_ / r_) + 0.5) * r_;
        bhh->i_.y_ = ((int) (bhh->i_.y_ / r_) + 0.5) * r_;
    }

    Point i[4];
    Line l = G::line(bhh->prev_, this);

    while ((fabs(this->x_ - bhh->i_.x_) > r_ / 2) || (fabs(this->y_ - bhh->i_.y_) > r_ / 2)) {
        i[Up].x_ = bhh->i_.x_;
        i[Up].y_ = bhh->i_.y_ + r_;
        i[Left].x_ = bhh->i_.x_ - r_;
        i[Left].y_ = bhh->i_.y_;
        i[Down].x_ = bhh->i_.x_;
        i[Down].y_ = bhh->i_.y_ - r_;
        i[Right].x_ = bhh->i_.x_ + r_;
        i[Right].y_ = bhh->i_.y_;

        int m = this->x_ > bhh->prev_.x_ ? Right : Left;
        int n = this->y_ > bhh->prev_.y_ ? Up : Down;

        if (G::distance(i[m], l) > G::distance(i[n], l)) m = n;
        data->addData(m);
        bhh->i_ = i[m];
        cmh->size() = 68 + ceil((double) data->size() / 4);        // 68 bytes is default of cmh and iph header
    }
}

// ------------------------ Create PolygonHole ------------------------ //

void
GridDynamicAgent::createGridHole(Packet *p, Point point) {
    gridHole *newhole = new gridHole();
    newhole->next_ = hole_;
    hole_ = newhole;

    // get a, x0, y0
    GridOfflinePacketData *data = (GridOfflinePacketData *) p->userdata();

    int nx = ceil(limit_x_ / r_);
    int ny = ceil(limit_y_ / r_);

    newhole->a_ = (bool **) malloc((nx) * sizeof(bool *));
    for (int i = 0; i < nx; i++) {
        newhole->a_[i] = (bool *) malloc((ny) * sizeof(bool));
    }
    for (int i = 0; i < nx; i++)
        for (int j = 0; j < ny; j++) {
            newhole->a_[i][j] = 0;
        }

    int x = point.x_ / r_;
    int y = point.y_ / r_;
    newhole->a_[x][y] = 1;
    for (int i = 1; i <= data->size(); i++) {
        switch (data->getData(i)) {
            case Up:
                y++;
                break;
            case Left:
                x--;
                break;
            case Down:
                y--;
                break;
            case Right:
                x++;
                break;
        }
        newhole->a_[x][y] = 1;
    }

    createPolygonHole(newhole);
}

bool
GridDynamicAgent::updateGridHole(Packet *p, gridHole *hole) {
    hdr_griddynamic *hdg = HDR_GRIDDYNAMIC(p);
    GridOfflinePacketData *data = (GridOfflinePacketData *) p->userdata();

    bool re = false;

    int x = hdg->p_.x_ / r_;
    int y = hdg->p_.y_ / r_;

    if (!hole->a_[x][y]) re = hole->a_[x][y] = 1;

    for (int i = 1; i <= data->size(); i++) {
        switch (data->getData(i)) {
            case Up:
                y++;
                break;
            case Left:
                x--;
                break;
            case Down:
                y--;
                break;
            case Right:
                x++;
                break;
        }

        if (!hole->a_[x][y]) re = hole->a_[x][y] = 1;
    }

    if (re) {
        createPolygonHole(hole);
    }

    return re;
}

void
GridDynamicAgent::createPolygonHole(gridHole *hole) {
    hole->clearNodeList();

    int nx = ceil(limit_x_ / r_);
    int ny = ceil(limit_y_ / r_);

    // find the leftist cell that painted in the lowest row
    int y = 0;
    int x = nx - 1;
    do {
        x = nx - 1;
        while (x >= 0 && hole->a_[x][y] == 0) x--;
        if (x >= 0) break;
        y++;
    } while (y < ny);

    node *sNode = new node();
    sNode->x_ = (x + 1) * r_;
    sNode->y_ = y * r_;
    sNode->next_ = hole->node_list_;
    hole->node_list_ = sNode;

    while (x >= 0 && hole->a_[x][y] == 1)
        x--; // find the end cell of serial painted cell from left to right in the lowest row
    x++;
    Point n, u;
    n.x_ = x * r_;
    n.y_ = y * r_;

    while (n.x_ != sNode->x_ || n.y_ != sNode->y_) {
        u = *(hole->node_list_);

        node *newNode = new node();
        newNode->x_ = n.x_;
        newNode->y_ = n.y_;
        newNode->next_ = hole->node_list_;
        hole->node_list_ = newNode;

        if (u.y_ == n.y_) {
            if (u.x_ < n.x_)        // >
            {
                if (y + 1 < ny && x + 1 < nx && hole->a_[x + 1][y + 1]) {
                    x += 1;
                    y += 1;
                    n.y_ += r_;
                }
                else if (x + 1 < nx && hole->a_[x + 1][y]) {
                    x += 1;
                    n.x_ += r_;
                    hole->node_list_ = newNode->next_;
                    delete newNode;
                }
                else {
                    n.y_ -= r_;
                }
            }
            else // u->x_ > v->x_			// <
            {
                if (y - 1 >= 0 && x - 1 >= 0 && hole->a_[x - 1][y - 1]) {
                    x -= 1;
                    y -= 1;
                    n.y_ -= r_;
                }
                else if (x - 1 >= 0 && hole->a_[x - 1][y]) {
                    x -= 1;
                    n.x_ -= r_;
                    hole->node_list_ = newNode->next_;
                    delete newNode;
                }
                else {
                    n.y_ += r_;
                }
            }
        }
        else    // u->x_ == v->x_
        {
            if (u.y_ < n.y_)        // ^
            {
                if (y + 1 < ny && x - 1 >= 0 && hole->a_[x - 1][y + 1]) {
                    x -= 1;
                    y += 1;
                    n.x_ -= r_;
                }
                else if (y + 1 < ny && hole->a_[x][y + 1]) {
                    y += 1;
                    n.y_ += r_;
                    hole->node_list_ = newNode->next_;
                    delete newNode;
                }
                else {
                    n.x_ += r_;
                }
            }
            else // u.x > n.x		// v
            {
                if (y - 1 >= 0 && x + 1 < nx && hole->a_[x + 1][y - 1]) {
                    x += 1;
                    y -= 1;
                    n.x_ += r_;
                }
                else if (y - 1 >= 0 && hole->a_[x][y - 1]) {
                    y -= 1;
                    n.y_ -= r_;
                    hole->node_list_ = newNode->next_;
                    delete newNode;
                }
                else {
                    n.x_ -= r_;
                }
            }
        }
    }

    // reduce polygon hole
    reducePolygonHole(hole);

    hole->circleNodeList();

    dumpBoundhole(hole);
}

void
GridDynamicAgent::reducePolygonHole(gridHole *h) {
    // printf("%f - reducePolygonHole\n", NOW);

    if (limit_ >= 4) {
        int count = 0;
        for (node *n = h->node_list_; n != NULL; n = n->next_) count++;

        //h->circleNodeList();
        node *temp = h->node_list_;
        while (temp->next_ && temp->next_ != h->node_list_) temp = temp->next_;
        temp->next_ = h->node_list_;

        // reduce hole
        node *gmin;
        int min;
        Point r;

        for (; count > limit_; count -= 2) {
            min = MAXINT;

            node *g = h->node_list_;
            do {
                node *g1 = g->next_;
                node *g2 = g1->next_;
                node *g3 = g2->next_;

                if (G::angle(g2, g1, g2, g3) > M_PI) {
                    int t = fabs(g3->x_ - g2->x_) * fabs(g2->y_ - g1->y_) +
                            fabs(g3->y_ - g2->y_) * fabs(g2->x_ - g1->x_);    // conditional is area
                    if (t < min) {
                        gmin = g;
                        min = t;
                        r.x_ = g1->x_ + g3->x_ - g2->x_;
                        r.y_ = g1->y_ + g3->y_ - g2->y_;
                    }
                }

                g = g1;
            }
            while (g != h->node_list_);

            if (r == *(gmin->next_->next_->next_->next_)) {
                node *temp = gmin->next_;
                gmin->next_ = gmin->next_->next_->next_->next_->next_;

                delete temp->next_->next_->next_;
                delete temp->next_->next_;
                delete temp->next_;
                delete temp;

                count -= 2;
            }
            else {
                node *newNode = new node();
                newNode->x_ = r.x_;
                newNode->y_ = r.y_;
                newNode->next_ = gmin->next_->next_->next_->next_;

                delete gmin->next_->next_->next_;
                delete gmin->next_->next_;
                delete gmin->next_;

                gmin->next_ = newNode;
            }

            h->node_list_ = gmin;
        }
    }
}

// ------------------------ Dump ------------------------ //

void
GridDynamicAgent::dumpBoundhole(gridHole *p) {
    FILE *fp = fopen("PolygonHole.tr", "a+");

    node *n = p->node_list_;
    do {
        fprintf(fp, "%f	%f\n", n->x_, n->y_);
        n = n->next_;
    } while (n && n != p->node_list_);

    fprintf(fp, "%f	%f\n\n", p->node_list_->x_, p->node_list_->y_);
    fclose(fp);
}

void GridDynamicAgent::dumpElection() {
    FILE *fp = fopen("Election.tr", "a+");
    fprintf(fp, "%f\t%d\t%f\t%f\n", NOW, my_id_, x_, y_);
    fclose(fp);
}

void GridDynamicAgent::dumpAlarm() {
    FILE *fp = fopen("Alarm.tr", "a+");
    fprintf(fp, "%f\t%d\t%f\t%f\n", NOW, my_id_, x_, y_);
    fclose(fp);
}

void GridDynamicAgent::dumpPivot() {
    FILE *fp = fopen("Pivot.tr", "a+");
    fprintf(fp, "%d - pivot (%f) %f %f\n", my_id_, NOW, x_, y_);
    double x = nx_ * r_;
    double y = ny_ * r_;
    fprintf(fp, "%f\t%f\n%f\t%f\n%f\t%f\n%f\t%f\n%f\t%f\n", x, y, x + r_, y, x + r_, y + r_, x, y + r_, x, y);
    fclose(fp);
}

void GridDynamicAgent::dumpCollect() {
    FILE *fp = fopen("Collect.tr", "a+");
    fprintf(fp, "%d - collect (%f)\n", my_id_, NOW);
    fclose(fp);
}

void GridDynamicAgent::dumpNodeOff(neighbor *n) {
    FILE *fp = fopen("NodeOff.tr", "a+");
    fprintf(fp, "%d - nodeoff:%d last time at %f(%f %f)-(%f)\n", my_id_, n->id_, n->time_, n->x_, n->y_, NOW);
    fclose(fp);
}

void GridDynamicAgent::dumpNodeOffReal() {
    if (NOW > 1000) return;
    FILE *fp = fopen("NodeOffReal.tr", "a+");
    fprintf(fp, "%d - %f %f %f\n", my_id_, x_, y_, NOW);
    fclose(fp);
}

void GridDynamicAgent::addTracking(nsaddr_t nid, Point location)
{
    neighbor *temp = NULL;
    for (neighbor *t = tracking_list_; t; t = (neighbor*)t->next_)
    {
        if (t->id_ == nid) {
            temp = t;
            break;
        }
    }

    if (temp == NULL)	 		// it is a new node
    {
        temp = new neighbor;
        temp->id_ 	= nid;
        temp->x_ 	= location.x_;
        temp->y_ 	= location.y_;
        temp->time_ = NOW;
        temp->next_ = NULL;

        if (tracking_list_ == NULL)	// the list now is empty
        {
            tracking_list_ = temp;
        }
        else	// the nodes list is not empty
        {
            node * i;
            for (i = tracking_list_; i->next_; i = i->next_)
            {
                if (temp->id_ > i->id_)
                {
                    temp->next_ = i->next_;
                    i->next_ = temp;
                    break;
                }
            }

            if (i->next_ == NULL)	// if angle is maximum, add temp to end of neighobrs list
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
