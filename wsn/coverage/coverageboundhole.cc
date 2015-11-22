//
// Created by eleven on 9/10/15.
//

#include "coverageboundhole.h"
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
    boundhole_node_list_ = NULL;
    cover_neighbors_ = NULL;
    sensor_neighbor_list_ = NULL;
    isBoundary = false;

    bind("range_", &communication_range_);
    bind("limit_boundhole_hop_", &limit_hop_);
    sensor_range_ = 0.5 * communication_range_;
}

int CoverageBoundHoleAgent::command(int argc, const char *const *argv) {
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

        node firstNode = data->get_intersect_data(0);
        node secondNode = data->get_intersect_data(1);
        if (firstNode.id_ == cmh->last_hop_ && secondNode.id_ == my_id_) {
            double min_y;
            node *first_circle = NULL;
            node *first_intersect = NULL;
            node *intersect_head = NULL;
            node *node_head = NULL;

            min_y = data->get_intersect_data(0).y_;
            bool flag = false;
            for (int i = 1; i < data->size(); i++) {
                node n_intersect = data->get_intersect_data(i);
                node *intersect = new node();
                intersect->x_ = n_intersect.x_;
                intersect->y_ = n_intersect.y_;
                intersect->id_ = n_intersect.id_;
                intersect->next_ = intersect_head;
                intersect_head = intersect;

                if (min_y > intersect->y_) {
                    first_intersect = intersect;
                    min_y = intersect->y_;
                    flag = true;
                }

                node n_node = data->get_node_data(i);
                node *node_ = new node();
                node_->x_ = n_node.x_;
                node_->y_ = n_node.y_;
                node_->id_ = n_node.id_;
                node_->next_ = node_head;
                node_head = node_;
                if (flag) {
                    first_circle = node_;
                }
                flag = false;
            }

            printf("start: %d\t%f\t%f\n", first_circle->id_, first_circle->x_, first_circle->y_);
            polygonHole *newHole = new polygonHole();
            newHole->node_list_ = intersect_head;
            newHole->next_ = hole_list_;
            hole_list_ = newHole;

            polygonHole *newNodeHole = new polygonHole();
            newNodeHole->node_list_ = node_head;
            newNodeHole->next_ = boundhole_node_list_;
            boundhole_node_list_ = newNodeHole;

            // circle boundhole list
            node *temp = newNodeHole->node_list_;
            while (temp->next_ && temp->next_ != newNodeHole->node_list_) temp = temp->next_;
            temp->next_ = newNodeHole->node_list_;

            // circle node list
            temp = newHole->node_list_;
            while (temp->next_ && temp->next_ != newHole->node_list_) temp = temp->next_;
            temp->next_ = newHole->node_list_;

            gridConstruction(newHole, first_circle, first_intersect);
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
        Packet *p;
        hdr_cmn *cmh;
        hdr_ip *iph;
        hdr_coverage *ch;

        for (stuckangle *sa = cover_neighbors_; sa; sa = sa->next_) {
            p = allocpkt();

            CoverageBoundHolePacketData *chpkt_data = new CoverageBoundHolePacketData();
            sensor_neighbor *n = getSensorNeighbor(sa->a_->id_);
            chpkt_data->add(n->id_, n->i2_.x_, n->i2_.y_, n->x_, n->y_);
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

void CoverageBoundHoleAgent::gridConstruction(polygonHole *hole,
                                              node *start_circle, node *start_intersect) {
    if (hole == NULL || hole->node_list_ == NULL) return;

    node *current_circle_a = NULL;
    node *current_intersect_a = NULL;
    triangle current_unit;
    struct limits limit;
    list *directions = NULL;
    DIRECTION prev_direction = DOWN; // trick: because we start from top so can not go up anymore

    limit.min_x = limit.max_x = hole->node_list_->x_;
    limit.min_y = limit.max_y = hole->node_list_->y_;
    for (node *tmp = hole->node_list_->next_; tmp != hole->node_list_; tmp = tmp->next_) {
        if (limit.min_x > tmp->x_) limit.min_x = tmp->x_;
        else if (limit.max_x < tmp->x_) limit.max_x = tmp->x_;
        if (limit.min_y > tmp->y_) limit.min_y = tmp->y_;
        else if (limit.max_y < tmp->y_) limit.max_y = tmp->y_;
    }

    current_circle_a = start_circle;
    current_intersect_a = start_intersect;

    int x_index = 0;
    int y_index = 0;

    x_index = (int) ((start_intersect->x_ - limit.min_x) / sensor_range_);
    current_unit.vertices[0].x_ = limit.min_x + x_index * sensor_range_;
    current_unit.vertices[0].y_ = limit.min_y;
    current_unit.vertices[1].x_ = current_unit.vertices[0].x_ + sensor_range_;
    current_unit.vertices[1].y_ = limit.min_y;
    current_unit.vertices[2].x_ = current_unit.vertices[0].x_ + sensor_range_ / 2;
    current_unit.vertices[2].y_ = limit.min_y + sensor_range_ * sqrt(3) / 2;

    debug(current_unit);

    while (current_circle_a->next_ != start_circle) {
        prev_direction = nextTriangle(&current_unit, &current_circle_a, &current_intersect_a, prev_direction);
        debug(current_unit);

        list *dir = new list();
        dir->e_ = prev_direction;
        dir->next_ = directions;
        directions = dir;
    }

    /* construct matrix */
    int nx = (int) ((limit.max_x - limit.min_x) / sensor_range_) + 1;
    int ny = (int) ((limit.max_y - limit.min_y) / (sensor_range_ * sqrt(3) / 2)) + 1;

    int8_t **grid = new int8_t *[nx];
    for (int i = 0; i < nx; i++)
        grid[i] = new int8_t[ny];

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            grid[i][j] = 0;
        }
    }

    grid[x_index][y_index] = 1;
    for (struct list *tmp = directions; tmp; tmp = tmp->next_) {
        switch (tmp->e_) {
            case RIGHT:
                x_index++;
                break;
            case LEFT:
                x_index--;
                break;
            case UP:
                y_index--;
                break;
            case DOWN:
                y_index++;
                break;
            default:
                break;
        }
        grid[x_index][y_index] = 1;
    }

    // free memory
    for (int i = 0; i < nx; i++)
        delete[] grid[i];
    delete[] grid;
}

/*----------------Utils function----------------------*/
void CoverageBoundHoleAgent::startUp() {
    FILE *fp;
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

/************** new *************/
DIRECTION CoverageBoundHoleAgent::nextTriangle(triangle *current_unit, node **circle_a,
                                               node **intersect_a,
                                               DIRECTION prev_direction) {
    triangle candidate;
    DIRECTION direction = NONE;
    DIRECTION tmp_direction = NONE;
    triangle prev_unit;
    for (int i = 0; i < 3; i++) {
        Point A, B, C;
        A = current_unit->vertices[i % 3];
        B = current_unit->vertices[(i + 1) % 3];
        C = current_unit->vertices[(i + 2) % 3];
        candidate.vertices[0] = A;
        candidate.vertices[1] = B;
        candidate.vertices[2].x_ = (A.x_ + B.x_) / 2 * 2 - C.x_;
        candidate.vertices[2].y_ = (A.y_ + B.y_) / 2 * 2 - C.y_;

        if (candidate.vertices[2].x_ < A.x_ && candidate.vertices[2].x_ < B.x_ &&
            candidate.vertices[2].x_ < C.x_)
            tmp_direction = LEFT;
        else if (candidate.vertices[2].x_ > A.x_ && candidate.vertices[2].x_ > B.x_ &&
                 candidate.vertices[2].x_ > C.x_)
            tmp_direction = RIGHT;
        else if (candidate.vertices[2].y_ > A.y_)
            tmp_direction = DOWN;
        else if (candidate.vertices[2].y_ < A.y_)
            tmp_direction = UP;

        if (tmp_direction + prev_direction == 0) {
            for (int j = 0; j < 3; j++) {
                prev_unit.vertices[j] = candidate.vertices[j];
            }
            continue;
        }

        if ((tmp_direction + prev_direction) != 0 &&
            isSelectableTriangle(*circle_a, *intersect_a, candidate)) {
            direction = tmp_direction;
            for (int j = 0; j < 3; j++) {
                current_unit->vertices[j] = candidate.vertices[j];
            }
            break;
        }
    }

    if (isOutdatedCircle((*intersect_a)->next_, *current_unit)) {
        *circle_a = (*circle_a)->next_;
        *intersect_a = (*intersect_a)->next_;
    }

    return direction;
}

bool CoverageBoundHoleAgent::isOutdatedCircle(node *intersect, triangle tri) {
    node *vertices = NULL;
    for (int i = 0; i < 3; i++) {
        node *node = new struct node();
        node->x_ = tri.vertices[i].x_;
        node->y_ = tri.vertices[i].y_;
        node->next_ = vertices;
        vertices = node;
    }
    return G::isPointInsidePolygon(intersect, vertices);
}

bool CoverageBoundHoleAgent::isSelectableTriangle(node *circle,
                                                  node *intersect_a,
                                                  triangle tri) {
    node *intersect_b = intersect_a->next_;

    Point center_circle;
    center_circle.x_ = circle->x_;
    center_circle.y_ = circle->y_;

    Point intersect1, intersect2;
    intersect1.x_ = intersect_a->x_;
    intersect1.y_ = intersect_a->y_;
    intersect2.x_ = intersect_b->x_;
    intersect2.y_ = intersect_b->y_;

    node *vertices = NULL;
    for (int i = 0; i < 3; i++) {
        node *node = new struct node();
        node->x_ = tri.vertices[i].x_;
        node->y_ = tri.vertices[i].y_;
        node->next_ = vertices;
        vertices = node;
    }

    if (G::isPointReallyInsidePolygon(&intersect1, vertices) ||
        G::isPointReallyInsidePolygon(&intersect2, vertices)) {
        return true;
    }

    for (int i = 0; i < 3; i++) {
        if (G::distance(tri.vertices[i], center_circle) <= sensor_range_ &&
            G::distance(tri.vertices[(i + 1) % 3], center_circle) <= sensor_range_) {
            continue;
        }
        if (G::distance(tri.vertices[i], center_circle) > sensor_range_ &&
            G::distance(tri.vertices[(i + 1) % 3], center_circle) > sensor_range_) {
            continue;
        }

        G::circleLineIntersect(center_circle, sensor_range_,
                               tri.vertices[i], tri.vertices[(i + 1) % 3],
                               &intersect1, &intersect2);

        if ((intersect1.x_ - tri.vertices[i].x_) * (intersect1.x_ - tri.vertices[(i + 1) % 3].x_) <= 0) {
            if (G::orientation(intersect_a, intersect1, intersect_b) == 2) {
                return true;
            }
        }
        if ((intersect2.x_ - tri.vertices[i].x_) * (intersect2.x_ - tri.vertices[(i + 1) % 3].x_) <= 0) {
            if (G::orientation(intersect_a, intersect2, intersect_b) == 2) {
                return true;
            }
        }

    }

    return false;
}

void CoverageBoundHoleAgent::debug(triangle current_unit) {
    printf("%f\t%f\n", current_unit.vertices[0].x_, current_unit.vertices[0].y_);
    printf("%f\t%f\n", current_unit.vertices[1].x_, current_unit.vertices[1].y_);
    printf("%f\t%f\n", current_unit.vertices[2].x_, current_unit.vertices[2].y_);
    printf("%f\t%f\n", current_unit.vertices[0].x_, current_unit.vertices[0].y_);
    printf("%f\t%f\n\n", (current_unit.vertices[1].x_ + current_unit.vertices[2].x_) / 2,
           (current_unit.vertices[1].y_ + current_unit.vertices[2].y_) / 2);

}