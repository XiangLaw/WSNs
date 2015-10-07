//
// Created by eleven on 9/10/15.
//

#include "coverageboundhole.h"
#include "../include/tcl.h"
#include "coverageboundhole_packet_data.h"


int hdr_coverage::offset_;

/*
 * Coverage Header Class
 */
static class CoverageBoundHoleHeaderClass : public PacketHeaderClass {
public:
    CoverageBoundHoleHeaderClass() : PacketHeaderClass("PacketHeader/COVERAGE", sizeof(hdr_coverage)) {
        bind_offset(&hdr_coverage::offset_);
    }

    ~CoverageBoundHoleHeaderClass() { }
} class_coveragehdr;

/*
 * Coverage Agent Class
 */
static class CoverageBoundHoleAgentClass : public TclClass {
public:
    CoverageBoundHoleAgentClass() : TclClass("Agent/COVERAGE") { }

    TclObject *create(int, const char *const *) {
        return (new CoverageBoundHoleAgent());
    }
} class_coverageboundhole;

void
CoverageBoundHoleTimer::expire(Event *e) {
    ((CoverageBoundHoleAgent*)a_->*firing_)();
}

// ------------------------ Agent ------------------------ //
CoverageBoundHoleAgent::CoverageBoundHoleAgent() : GPSRAgent(),
                                                   boundhole_timer_(this, &CoverageBoundHoleAgent::holeBoundaryDetection) {
    hole_list_ = NULL;
    cover_neighbors_ = NULL;
    sensor_neighbor_list_ = NULL;

    bind("range_", &communication_range_);
    bind("limit_boundhole_hop_", &limit_hop);
//    bind("sensor_range_", &sensor_range_);
    sensor_range_ = 0.5 * communication_range_;
}

int CoverageBoundHoleAgent::command(int argc, const char *const *argv) {
    if (strcasecmp(argv[1], "start") == 0) {
        startUp();
    } else if (strcasecmp(argv[1], "dump") == 0) {
        dumpSensorNeighbor();
    } else if (strcasecmp(argv[1], "coverage") == 0) {
        boundaryNodeDetection();
        boundhole_timer_.resched(10 + randSend_.uniform(0.0, 5));
        return TCL_OK;
    }

    return GPSRAgent::command(argc, argv);
}

void CoverageBoundHoleAgent::recv(Packet *p, Handler *h) {
    struct hdr_cmn *cmh = HDR_CMN(p);
    struct hdr_ip *iph = HDR_IP(p);

    switch (cmh->ptype()) {
        case PT_COVERAGE:
            recvCoverage(p);
            break;
        default:
            GPSRAgent::recv(p, h);
    }
}

void CoverageBoundHoleAgent::recvCoverage(Packet *p) {
    struct hdr_ip *iph = HDR_IP(p);
    struct hdr_cmn *cmh = HDR_CMN(p);

    CoverageBoundHolePacketData *data = (CoverageBoundHolePacketData *) p->userdata();

    // if the boundhole packet has came back to the initial node
    if (iph->saddr() == my_id_) {
        if (data->size() == 0) {
            drop(p);
            return;
        }

        node firstNode = data->get_data(0);
        if (firstNode.id_ == cmh->last_hop_) {
                node* head = NULL;
                for (int i = 0; i < data->size(); i++){
                    node n = data->get_data(i);
                    node *item = new node();
                    item->x_ = n.x_;
                    item->y_ = n.y_;
                    item->id_ = n.id_;
                    item->next_ = head;
                    head = item;
                }
                polygonHole* newHole = new polygonHole();
                newHole->node_list_ = head;
                newHole->next_ = hole_list_;
                hole_list_ = newHole;

                data->dump();
                printf("%d source\n", iph->src());
                printf("%d Boundhole is detected, nodeNumberEstimation: %d\n", my_id_, nodeNumberEstimation(newHole));
                drop(p, "COVERAGE_BOUNDHOLE");
            return;
        }
    }

    if (iph->ttl_-- <= 0) {
        drop(p, DROP_RTR_TTL);
        return;
    }

    if (iph->saddr() < my_id_) {
        drop(p, " REPEAT");
        return;
    }

    node *nb = getNextSensorNeighbor(cmh->last_hop_);
    if (nb == NULL) {
        drop(p, DROP_RTR_NO_ROUTE);
        return;
    }

    sensor_neighbor *n = getSensorNeighbor(cmh->last_hop_);
    data->add(n->id_, n->i2_.x_, n->i2_.y_);
    int k = data->size();
    int m = data->get_data(1).id_;

    if (cmh->uid() == 658){
        int i = 0;
    }

    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = nb->id_;
    cmh->last_hop_ = my_id_;

    iph->daddr() = nb->id_;

    send(p, 0);
}

/*----------------------------BOUNDARY DETECTION------------------------------------------------------*/
bool CoverageBoundHoleAgent::boundaryNodeDetection() {
    Point c1, c2;
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

        if (G::distance(temp->i2_, next) > sensor_range_) {
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

void CoverageBoundHoleAgent::holeBoundaryDetection() {
    if (isBoundary) {
        dumpBoundaryDetect();
        Packet *p;
        hdr_cmn *cmh;
        hdr_ip *iph;
        hdr_coverage *ch;

        for (stuckangle *sa = cover_neighbors_; sa; sa = sa->next_) {
            p = allocpkt();

            CoverageBoundHolePacketData *chpkt_data = new CoverageBoundHolePacketData();
            sensor_neighbor* n = getSensorNeighbor(sa->a_->id_);
            chpkt_data->add(n->id_, n->i2_.x_, n->i2_.y_);
            p->setdata(chpkt_data);

            cmh = HDR_CMN(p);
            iph = HDR_IP(p);
            ch = HDR_COVERAGE(p);

            cmh->ptype() = PT_COVERAGE;
            cmh->direction() = hdr_cmn::DOWN;
            cmh->size() += IP_HDR_LEN + ch->size() + chpkt_data->data_len_;
            cmh->next_hop_ = sa->b_->id_;
            cmh->last_hop_ = my_id_;
            cmh->addr_type_ = NS_AF_INET;

            iph->saddr() = my_id_;
            iph->daddr() = sa->b_->id_;
            iph->sport() = RT_PORT;
            iph->dport() = RT_PORT;
            iph->ttl_ = limit_hop;            // more than ttl_ hop => boundary => remove

            send(p, 0);

//            printf("%d\t- Send Coverage to %d(%d)\n", my_id_, cmh->next_hop(),cmh->uid());
        }
    }
}

int CoverageBoundHoleAgent::nodeNumberEstimation(polygonHole *pHole) {
    if (pHole == NULL || pHole->node_list_ == NULL) return 0;

    // find minx, maxx, miny, maxy
    double minx, maxx, miny, maxy;
    minx = maxx = pHole->node_list_->x_;
    miny = maxy = pHole->node_list_->y_;
    for(node* tmp = pHole->node_list_->next_; tmp; tmp = tmp->next_) {
        if (minx > tmp->x_) minx = tmp->x_;
        else if (maxx < tmp->x_) maxx = tmp->x_;
        if (miny > tmp->y_) miny = tmp->y_;
        else if (maxy < tmp->y_) maxy = tmp->y_;
    }

    node* node_list_ = NULL, *tail_node_;
    for(node* tmp = pHole->node_list_; tmp; tmp = tmp->next_){
        node* item = new node();
        item->id_ = tmp->id_;
        item->x_ = tmp->x_ - minx;
        item->y_ = tmp->y_ - miny;
        item->next_ = NULL;
        if (node_list_ == NULL){
            node_list_ = item;
        } else {
            tail_node_->next_ = item;
        }
        tail_node_ = item;
    }

    Point prev_cell_ = *node_list_;
    double r_ = sensor_range_*sqrt(2);

    if (fmod(prev_cell_.x_, r_) == 0)    // i lies in vertical line
    {
        if (this->x_ > prev_cell_.x_) prev_cell_.x_ += r_ / 2;
        else if (this->x_ < prev_cell_.x_) prev_cell_.x_ -= r_ / 2;
        else // (this->x_ == prev_cell_.x_)
        {
            if (this->y_ > prev_cell_.y_) prev_cell_.x_ += r_ / 2;
            else prev_cell_.x_ -= r_ / 2;
        }
    }
    if (fmod(prev_cell_.y_, r_) == 0)    // i lies in h line
    {
        if (this->y_ > prev_cell_.y_) prev_cell_.y_ += r_ / 2;
        else if (this->y_ < prev_cell_.y_) prev_cell_.y_ -= r_ / 2;
        else // (this->y_ == prev_cell_.y_)
        {
            if (this->x_ > prev_cell_.x_) prev_cell_.y_ -= r_ / 2;
            else prev_cell_.y_ += r_ / 2;
        }
    }

    prev_cell_.x_ = ((int) (prev_cell_.x_ / r_) + 0.5) * r_;
    prev_cell_.y_ = ((int) (prev_cell_.y_ / r_) + 0.5) * r_;

    struct list *head = NULL, *tail;
    for (node* tmp = node_list_; tmp; tmp = tmp->next_){
        node* cur = tmp->next_ == NULL ? node_list_ : tmp->next_;
        Point i[4];
        Line l = G::line(tmp, cur);

        while ((fabs(cur->x_ - prev_cell_.x_) > r_ / 2) || (fabs(cur->y_ - prev_cell_.y_) > r_ / 2)) {
            i[Up].x_ = prev_cell_.x_;
            i[Up].y_ = prev_cell_.y_ + r_;
            i[Left].x_ = prev_cell_.x_ - r_;
            i[Left].y_ = prev_cell_.y_;
            i[Down].x_ = prev_cell_.x_;
            i[Down].y_ = prev_cell_.y_ - r_;
            i[Right].x_ = prev_cell_.x_ + r_;
            i[Right].y_ = prev_cell_.y_;

            int m = cur->x_ > tmp->x_ ? Right : Left;
            int n = cur->y_ > tmp->y_ ? Up : Down;

            if (G::distance(i[m], l) > G::distance(i[n], l)) m = n;
            prev_cell_ = i[m];
            struct list* item = new list();
            item->e_ = m;
            item->next_ = NULL;
            if (head == NULL){
                head = item;
            } else {
                tail->next_ = item;
            }
            tail = item;
        }
    }

    int nx = floor((maxx - minx)/r_) + 1;
    int ny = floor((maxy - miny)/r_) + 1;
    bool **a = (bool **) malloc((nx) * sizeof(bool *));
    for (int i = 0; i < nx; i++)
        a[i] = (bool *) malloc((ny) * sizeof(bool));

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            a[i][j] = 0;
        }
    }

    int x = floor(node_list_->x_/r_);
    int y = floor(node_list_->y_/r_);
    a[x][y] = 1;
    for (struct list* tmp = head; tmp; tmp=tmp->next_) {
        switch (tmp->e_) {
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
        a[x][y] = 1;
    }

    int count = 0;
    for (int i = 0; i < nx; i++) {
        int j = 0;
        while (j < ny) {
            while (!a[i][j] && j < ny) j++;
            while (a[i][j] && j < ny) {
                j++;
                count++;
            }
        }
    }

    return count;
}

/*----------------Utils function----------------------*/

void
CoverageBoundHoleAgent::startUp() {
    FILE *fp;
    fp = fopen("SensorNeighbors.tr", "w");
    fp = fopen("NodeBoundaryDetect.tr", "w");
    fp = fopen("CoverageBoundHole.tr", "w");
    fclose(fp);
}

void CoverageBoundHoleAgent::addNeighbor(nsaddr_t addr, Point p) {
    GPSRAgent::addNeighbor(addr, p);

    addSensorNeighbor(addr, p, 2); // fixed add level 2 sensor neighbors
}

void CoverageBoundHoleAgent::addSensorNeighbor(nsaddr_t nid, Point location, int level) {
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
            sensor_neighbor *i, *next, *i2;

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
                    Angle ai_ = G::angle(*this, i->i1_,*this, i->i2_);
                    if (G::angle(*this, i->i1_, *this, temp->i1_) < ai_
                        && G::angle(*this, i->i1_, *this, temp->i2_) < ai_){
                        break;
                    } else {
                        i->next_ = temp;
                        Angle atemp_ = G::angle(*this, temp->i1_,*this, temp->i2_);
                        for (i2 = next; i2; i2 = (sensor_neighbor *) i2->next_){
                            if (G::angle(*this, temp->i1_,*this, i2->i1_) < atemp_
                                && G::angle(*this, temp->i1_,*this, i2->i2_) < atemp_){
                                continue;
                            } else {
                                break;
                            }
                        }

                        temp->next_ = i2;
                        if (i2 == NULL){
                            for (i2 = sensor_neighbor_list_; i2; i2 = (sensor_neighbor *) i2->next_){
                                if (G::angle(*this, temp->i1_,*this, i2->i1_) < atemp_
                                    && G::angle(*this, temp->i1_,*this, i2->i2_) < atemp_){
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
                Angle ai_ = G::angle(*this, i->i1_,*this, i->i2_);
                if (G::angle(*this, i->i1_, *this, temp->i1_) < ai_
                    && G::angle(*this, i->i1_, *this, temp->i2_) < ai_){
                } else {
                    i->next_ = temp;
                    Angle atemp_ = G::angle(*this, temp->i1_,*this, temp->i2_);
                    for (i2 = sensor_neighbor_list_; i2; i2 = (sensor_neighbor *) i2->next_){
                        if (G::angle(*this, temp->i1_,*this, i2->i1_) < atemp_
                            && G::angle(*this, temp->i1_,*this, i2->i2_) < atemp_){
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

sensor_neighbor *CoverageBoundHoleAgent::getSensorNeighbor(nsaddr_t addr) {
    for (sensor_neighbor *temp = sensor_neighbor_list_; temp; temp = (sensor_neighbor *) temp->next_) {
        if (temp->id_ == addr) return temp;
    }
    return NULL;
}

node *CoverageBoundHoleAgent::getNextSensorNeighbor(nsaddr_t prev_node) {
    for (stuckangle *pair = cover_neighbors_; pair; pair = pair->next_) {
        if (prev_node == pair->a_->id_) return pair->b_;
    }
    return NULL;
}

/*----------------- DUMP --------------------------*/
void CoverageBoundHoleAgent::dumpSensorNeighbor() {
    FILE *fp = fopen("SensorNeighbors.tr", "a");
    fprintf(fp, "%d	%f\t%f\t", this->my_id_, this->x_, this->y_);
    for (sensor_neighbor *temp = sensor_neighbor_list_; temp; temp = (sensor_neighbor *) temp->next_) {
        fprintf(fp, "%d(%f,%f\t%f,%f),", temp->id_, temp->i1_.x_, temp->i1_.y_, temp->i2_.x_, temp->i2_.y_);
    }
    fprintf(fp, "\n");
    fclose(fp);
}

void CoverageBoundHoleAgent::dumpBoundaryDetect(){
    FILE* fp;
    fp = fopen("NodeBoundaryDetect.tr", "a");
    fprintf(fp, "%d\t%f\t%f", my_id_, x_, y_);
    for (stuckangle *pair = cover_neighbors_; pair; pair = pair->next_) {
        fprintf(fp,"\t%d-%d",pair->a_->id_,pair->b_->id_);
    }
    fprintf(fp,"\n");
    fclose(fp);
}
