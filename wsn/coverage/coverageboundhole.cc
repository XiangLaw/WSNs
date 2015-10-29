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
    ((CoverageBoundHoleAgent *) a_->*firing_)();
}

// ------------------------ Agent ------------------------ //
CoverageBoundHoleAgent::CoverageBoundHoleAgent() : GPSRAgent(),
                                                   boundhole_timer_(this,
                                                                    &CoverageBoundHoleAgent::holeBoundaryDetection) {
    hole_list_ = NULL;
    cover_neighbors_ = NULL;
    sensor_neighbor_list_ = NULL;

    bind("range_", &communication_range_);
    bind("limit_boundhole_hop_", &limit_hop_);
//    bind("sensor_range_", &sensor_range_);
    sensor_range_ = 0.5 * communication_range_;
}

int CoverageBoundHoleAgent::command(int argc, const char *const *argv) {
    if (strcasecmp(argv[1], "start") == 0) {
        startUp();
    } else if (strcasecmp(argv[1], "dump") == 0) {
        dumpSensorNeighbor();
    } else if (strcasecmp(argv[1], "coverage") == 0) {
        runTimeCounter.start();
        boundaryNodeDetection();
        boundhole_timer_.resched(10 + randSend_.uniform(0.0, 5));
        return TCL_OK;
    }

    return GPSRAgent::command(argc, argv);
}

void CoverageBoundHoleAgent::recv(Packet *p, Handler *h) {
    struct hdr_cmn *cmh = HDR_CMN(p);

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
        if (data->size() < 3) {
            drop(p);
            return;
        }

        node firstNode = data->get_data(0);
        node secondNode = data->get_data(1);
        if (firstNode.id_ == cmh->last_hop_ && secondNode.id_ == my_id_) {
            node *head = NULL;
            for (int i = 0; i < data->size(); i++) {
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

            data->dump();
            gridConstruction(newHole);
            drop(p, "COVERAGE_BOUNDHOLE");
            dumpCoverageBoundHole(newHole);
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
    data->add(n->id_, n->i2_.x_, n->i2_.y_);

    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = nb->id_;
    cmh->last_hop_ = my_id_;

    iph->daddr() = nb->id_;

    send(p, 0);
}

/*----------------------------BOUNDARY DETECTION------------------------------------------------------*/
bool CoverageBoundHoleAgent::boundaryNodeDetection() {
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
            sensor_neighbor *n = getSensorNeighbor(sa->a_->id_);
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
            iph->ttl_ = limit_hop_;            // more than ttl_ hop => boundary => remove

            send(p, 0);
        }
    }
}

void CoverageBoundHoleAgent::gridConstruction(polygonHole *newHole) {
    if (newHole == NULL || newHole->node_list_ == NULL) return;

    // find minx, maxx, miny, maxy
    double minx, maxx, miny, maxy;
    minx = maxx = newHole->node_list_->x_;
    miny = maxy = newHole->node_list_->y_;
    for (node *tmp = newHole->node_list_->next_; tmp; tmp = tmp->next_) {
        if (minx > tmp->x_) minx = tmp->x_;
        else if (maxx < tmp->x_) maxx = tmp->x_;
        if (miny > tmp->y_) miny = tmp->y_;
        else if (maxy < tmp->y_) maxy = tmp->y_;
    }

    node *node_list_ = NULL, *tail_node_ = NULL;
    for (node *tmp = newHole->node_list_; tmp; tmp = tmp->next_) {
        node *item = new node();
        item->id_ = tmp->id_;
        item->x_ = tmp->x_ - minx;
        item->y_ = tmp->y_ - miny;
        item->next_ = NULL;
        if (node_list_ == NULL) {
            node_list_ = item;
        } else {
            tail_node_->next_ = item;
        }
        tail_node_ = item;
    }

    Point prev_cell_ = *node_list_;
    double r_ = sensor_range_ * sqrt(2) / 2;

    /*if (fmod(prev_cell_.x_, r_) == 0)    // i lies in vertical line
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
    }*/
    if (fmod(prev_cell_.x_, r_) == 0)    // i lies in vertical line
    {
        prev_cell_.x_ -= r_ / 2;
    }
    if (fmod(prev_cell_.y_, r_) == 0)    // i lies in h line
    {
        prev_cell_.y_ += r_ / 2;
    }

    prev_cell_.x_ = ((int) (prev_cell_.x_ / r_) + 0.5) * r_;
    prev_cell_.y_ = ((int) (prev_cell_.y_ / r_) + 0.5) * r_;

    struct list *head = NULL, *tail = NULL;
    for (node *tmp = node_list_; tmp; tmp = tmp->next_) {
        node *cur = tmp->next_ == NULL ? node_list_ : tmp->next_;
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
            struct list *item = new list();
            item->e_ = m;
            item->next_ = NULL;
            if (head == NULL) {
                head = item;
            } else {
                tail->next_ = item;
            }
            tail = item;
        }
    }

    /* construct array of color of grid */
    int nx = (int) floor((maxx - minx) / r_) + 1;
    int ny = (int) floor((maxy - miny) / r_) + 1;

    int8_t **a = new int8_t *[nx + 1];
    for (int i = 0; i < nx + 1; i++)
        a[i] = new int8_t[ny + 1];

    for (int i = 0; i < nx + 1; i++) {
        for (int j = 0; j < ny + 1; j++) {
            a[i][j] = 0;
        }
    }

    int x = (int) floor(node_list_->x_ / r_);
    int y = (int) floor(node_list_->y_ / r_);
    a[x][y] = 1;
    for (struct list *tmp = head; tmp; tmp = tmp->next_) {
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
            default:
                break;
        }
        a[x][y] = 1;
    } // done. a array is now grid boundary

    newHole->node_list_ = NULL;
    x = nx - 1;
    y = 0;
    while (x >= 0 && a[x][0] == 0) x--;
    node *sNode = new node();
    sNode->x_ = minx + (x + 1) * r_;
    sNode->y_ = miny;
    sNode->next_ = newHole->node_list_;
    newHole->node_list_ = sNode;

    /* construct grid boundary */
    while (x >= 0 && a[x][0] == 1) x--; // find the end cell of serial painted cell from left to right in the lowest row
    x++;
    Point n, u;
    n.x_ = minx + x * r_;
    n.y_ = miny;

    while (n.x_ != sNode->x_ || n.y_ != sNode->y_) {
        u = *(newHole->node_list_);

        node *newNode = new node();
        newNode->x_ = n.x_;
        newNode->y_ = n.y_;
        newNode->next_ = newHole->node_list_;
        newHole->node_list_ = newNode;

        if (u.y_ == n.y_) {
            if (u.x_ < n.x_)        // >
            {
                if (y + 1 < ny && x + 1 < nx && a[x + 1][y + 1]) {
                    x += 1;
                    y += 1;
                    n.y_ += r_;
                }
                else if (x + 1 < nx && a[x + 1][y]) {
                    x += 1;
                    n.x_ += r_;
                    newHole->node_list_ = newNode->next_;
                    delete newNode;
                }
                else {
                    n.y_ -= r_;
                }
            }
            else // u->x_ > v->x_			// <
            {
                if (y - 1 >= 0 && x - 1 >= 0 && a[x - 1][y - 1]) {
                    x -= 1;
                    y -= 1;
                    n.y_ -= r_;
                }
                else if (x - 1 >= 0 && a[x - 1][y]) {
                    x -= 1;
                    n.x_ -= r_;
                    newHole->node_list_ = newNode->next_;
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
                if (y + 1 < ny && x - 1 >= 0 && a[x - 1][y + 1]) {
                    x -= 1;
                    y += 1;
                    n.x_ -= r_;
                }
                else if (y + 1 < ny && a[x][y + 1]) {
                    y += 1;
                    n.y_ += r_;
                    newHole->node_list_ = newNode->next_;
                    delete newNode;
                }
                else {
                    n.x_ += r_;
                }
            }
            else // u.x > n.x		// v
            {
                if (y - 1 >= 0 && x + 1 < nx && a[x + 1][y - 1]) {
                    x += 1;
                    y -= 1;
                    n.x_ += r_;
                }
                else if (y - 1 >= 0 && a[x][y - 1]) {
                    y -= 1;
                    n.y_ -= r_;
                    newHole->node_list_ = newNode->next_;
                    delete newNode;
                }
                else {
                    n.x_ -= r_;
                }
            }
        }
    }
//
//    reducePolygonHole(newHole);
    patchingHole(newHole, minx, miny, r_, a, nx, ny);
    // free memory
    for (int i = 0; i < nx; i++)
        delete[] a[i];
    delete[] a;
}

void CoverageBoundHoleAgent::patchingHole(polygonHole *hole, double base_x, double base_y, double r_,
                                          int8_t **grid, int nx, int ny) {
    int x = 0;
    int y = 0;
    // fill the grid with color
    for (y = 1; y < ny - 1; y++) {
        for (x = 1; x < nx - 1; x++) {
            if (grid[x][y] == C_WHITE || grid[x - 1][y] != C_WHITE || grid[x][y - 1] != C_WHITE) continue;
            int flag = 0;
            for (int i = x + 1; i < nx; i++) {
                if (grid[i][y] == C_WHITE) {
                    flag++;
                    break;
                }
            }
            for (int i = y + 1; i < ny; i++) {
                if (grid[x][i] == C_WHITE) {
                    flag++;
                    break;
                }
            }
            if (flag >= 2) grid[x][y] = C_WHITE;
        }
    }

    x = 0;
    y = 0;
    Point patching_point;
    while (x < nx) {
        if (white_node_count(grid[x][y], grid[x + 1][y], grid[x][y + 1], grid[x + 1][y + 1]) >= 3) {
            grid[x][y] = grid[x + 1][y] = grid[x][y + 1] = grid[x + 1][y + 1] = C_RED;
            patching_point.x_ = base_x + (x + 1) * r_;
            patching_point.y_ = base_y + (y + 1) * r_;
            dumpPatchingHole(patching_point);
            y += 2;
        }
        else {
            if ((x - 1 > 0) &&
                (white_node_count(grid[x][y], grid[x][y + 1], grid[x - 1][y], grid[x - 1][y + 1]) >= 3)) {
                grid[x][y] = grid[x - 1][y] = grid[x][y + 1] = grid[x - 1][y + 1] = C_RED;
                patching_point.x_ = base_x + x * r_;
                patching_point.y_ = base_y + (y + 1) * r_;
                dumpPatchingHole(patching_point);
                y += 2;
            }
            else {
                y += 1;
            }
        }
        if (y > ny) {
            x += 2;
            y = 0;
        }
    }

    for (x = 0; x < nx; x++) {
        for (y = 0; y < ny; y++) {
            if (grid[x][y] != C_WHITE) continue;
            if (grid[x + 1][y] == C_WHITE) { // (m, 1)
                int size = 0;
                for (; grid[x + size][y] == C_WHITE; size++) {
                    grid[x + size][y] = C_RED;
                }
                size = (int) floor(size / sqrt(7)) + 1;
                for (int i = 0; i < size; i++) {
                    patching_point.x_ = base_x + (x) * r_ + r_ * sqrt(7) / 2 + i * r_ * sqrt(7);
                    patching_point.y_ = base_y + (y) * r_ + r_ / 2;
                    dumpPatchingHole(patching_point);
                }
            }
            else if (grid[x][y + 1] == C_WHITE) { // (1, m)
                int size = 0;
                for (; grid[x][y + size] == C_WHITE; size++) {
                    grid[x][y + size] = C_RED;
                }
                size = (int) floor(size / sqrt(7)) + 1;
                for (int i = 0; i < size; i++) {
                    patching_point.x_ = base_x + (x) * r_ + r_ / 2;
                    patching_point.y_ = base_y + (y) * r_ + r_ * sqrt(7) / 2 + i * r_ * sqrt(7);
                    dumpPatchingHole(patching_point);
                }
            }
            else {
                patching_point.x_ = base_x + x * r_;
                patching_point.y_ = base_y + y * r_;
                dumpPatchingHole(patching_point);
                grid[x][y] = C_RED;
            }
        }
    }

    for (int j = ny; j >= 0; j--) {
        for (int i = 0; i < nx; i++)
            printf("%d", grid[i][j]);
        printf("\n");
    }

}

int CoverageBoundHoleAgent::white_node_count(int a, int b, int c, int d) {
    int count = 0;
    if (a == C_WHITE) count++;
    if (b == C_WHITE) count++;
    if (c == C_WHITE) count++;
    if (d == C_WHITE) count++;
    return count;
}

/*----------------Utils function----------------------*/
void CoverageBoundHoleAgent::startUp() {
    FILE *fp;
    fp = fopen("SensorNeighbors.tr", "w");
    fclose(fp);
    fp = fopen("NodeBoundaryDetect.tr", "w");
    fclose(fp);
    fp = fopen("CoverageBoundHole.tr", "w");
    fclose(fp);
    fp = fopen("CoverageGrid.tr", "w");
    fclose(fp);
    fp = fopen("PatchingHole.tr", "w");
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

void CoverageBoundHoleAgent::reducePolygonHole(polygonHole *h) {
//    int count = 0;
//    for (node *n = h->node_list_; n != NULL; n = n->next_) count++;
//
//    //h->circleNodeList();
//    node *temp = h->node_list_;
//    while (temp->next_ && temp->next_ != h->node_list_) temp = temp->next_;
//    temp->next_ = h->node_list_;
//
//    // reduce hole
//    node *gmin;
//    int min;
//    Point r;
//
//    for (; count > limit_; count -= 2) {
//        min = MAXINT;
//
//        node *g = h->node_list_;
//        do {
//            node *g1 = g->next_;
//            node *g2 = g1->next_;
//            node *g3 = g2->next_;
//
//            if (G::angle(g2, g1, g2, g3) > M_PI) {
//                //	int t = fabs(g3->x_ - g2->x_) + fabs(g2->y_ - g1->y_) + fabs(g3->y_ - g2->y_) + fabs(g2->x_ - g1->x_);	// conditional is boundary length
//                int t = fabs(g3->x_ - g2->x_) * fabs(g2->y_ - g1->y_) + fabs(g3->y_ - g2->y_) * fabs(g2->x_ - g1->x_);    // conditional is area
//                if (t < min) {
//                    gmin = g;
//                    min = t;
//                    r.x_ = g1->x_ + g3->x_ - g2->x_;
//                    r.y_ = g1->y_ + g3->y_ - g2->y_;
//                }
//            }
//
//            g = g1;
//        }
//        while (g != h->node_list_);
//
//        if (r == *(gmin->next_->next_->next_->next_)) {
//            node *temp = gmin->next_;
//            gmin->next_ = gmin->next_->next_->next_->next_->next_;
//
//            delete temp->next_->next_->next_;
//            delete temp->next_->next_;
//            delete temp->next_;
//            delete temp;
//
//            count -= 2;
//        }
//        else {
//            node *newNode = new node();
//            newNode->x_ = r.x_;
//            newNode->y_ = r.y_;
//            newNode->next_ = gmin->next_->next_->next_->next_;
//
//            delete gmin->next_->next_->next_;
//            delete gmin->next_->next_;
//            delete gmin->next_;
//
//            gmin->next_ = newNode;
//        }
//
//        h->node_list_ = gmin;
//    }
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

void CoverageBoundHoleAgent::dumpBoundaryDetect() {
    FILE *fp;
    fp = fopen("NodeBoundaryDetect.tr", "a");
    fprintf(fp, "%d\t%f\t%f", my_id_, x_, y_);
    for (stuckangle *pair = cover_neighbors_; pair; pair = pair->next_) {
        fprintf(fp, "\t%d-%d", pair->a_->id_, pair->b_->id_);
    }
    fprintf(fp, "\n");
    fclose(fp);
}

void CoverageBoundHoleAgent::dumpCoverageBoundHole(polygonHole *pHole) {
    FILE *fp;
    fp = fopen("CoverageGrid.tr", "a");
    for (node *temp = pHole->node_list_; temp; temp = temp->next_) {
        fprintf(fp, "%f\t%f\n", temp->x_, temp->y_);
    }
    fprintf(fp, "%f\t%f\n", pHole->node_list_->x_, pHole->node_list_->y_);
    fprintf(fp, "\n");
    fclose(fp);
}

void CoverageBoundHoleAgent::dumpPatchingHole(Point point) {
    FILE *fp;
    fp = fopen("PatchingHole.tr", "a");
    fprintf(fp, "%f\t%f\n", point.x_, point.y_);
    fclose(fp);
}
