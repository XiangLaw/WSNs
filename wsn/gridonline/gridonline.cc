/*
 * grid.cc
 *
 *  Last edited on Nov 14, 2013
 *  by Trong Nguyen
 */

#include "config.h"
#include "gridonline.h"
#include "gridonline_packet_data.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#include "../include/tcl.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define Up 0
#define Left 1
#define Down 2
#define Right 3

static class GridOnlineHeaderClass : public PacketHeaderClass
{
public:
    GridOnlineHeaderClass() : PacketHeaderClass("PacketHeader/GRID", sizeof(hdr_grid))
    {
        bind_offset(&hdr_grid::offset_);
    }
}class_gridhdr;

static class GridOnlineAgentClass : public TclClass
{
public:
    GridOnlineAgentClass() : TclClass("Agent/GRIDONLINE") {}
    TclObject* create(int, const char*const*)
    {
        return new GridOnlineAgent();
    }
}class_grid;

void
GridOnlineTimer::expire(Event *e) {
    (a_->*firing_)();
}

// ------------------------ Agent ------------------------ //

GridOnlineAgent::GridOnlineAgent() : GPSRAgent(),
                                     findStuck_timer_(this, &GridOnlineAgent::findStuckAngle),
                                     grid_timer_(this, &GridOnlineAgent::sendBoundHole)
{
    stuck_angle_ = NULL;
    hole_list_ = NULL;
    bind("range_", &range_); // bán kính truyền tin trong mạng
    bind("limit_", &limit_); // limit?
    bind("r_", &r_); // grid size ?
    bind("limit_boundhole_hop_", &limit_boundhole_hop_);
}

int
GridOnlineAgent::command(int argc, const char*const* argv)
{
    if (argc == 2)
    {
        if (strcasecmp(argv[1], "start") == 0)
        {
            startUp();
        }
        if (strcasecmp(argv[1], "boundhole") == 0)
        {
            grid_timer_.resched(randSend_.uniform(0.0, 5));
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "routing") == 0)
        {
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "bcenergy") == 0)
        {
            dumpEnergy();
            return TCL_OK;
        }
    }

    return GPSRAgent::command(argc,argv);
}

// handle the receive packet just of type PT_GRID
void
GridOnlineAgent::recv(Packet *p, Handler *h)
{
    hdr_cmn *cmh = HDR_CMN(p);

    switch (cmh->ptype())
    {
        case PT_HELLO:
            GPSRAgent::recv(p, h);
            break;

        case PT_GRID:
            recvBoundHole(p);
            break;

        default:
            drop(p, " UnknowType");
            break;
    }
}

void
GridOnlineAgent::startUp()
{
    findStuck_timer_.resched(20);

    // clear trace file
    FILE *fp;
    fp = fopen("Area.tr", 		"w");		fclose(fp);
    fp = fopen("GridOnline.tr", 	"w");	fclose(fp);
    fp = fopen("Neighbors.tr", 	"w");		fclose(fp);
    fp = fopen("Time.tr", 		"w");		fclose(fp);
}

// ------------------------ Bound hole ------------------------ //

void
GridOnlineAgent::findStuckAngle()
{
    if (neighbor_list_ == NULL || neighbor_list_->next_ == NULL)
    {
        stuck_angle_ = NULL;
        return;
    }

    node *nb1 = neighbor_list_;
    node *nb2 = neighbor_list_->next_;

    while (nb2)
    {
        Circle circle = G::circumcenter(this, nb1, nb2);
        Angle a = G::angle(this, nb1, this, &circle);
        Angle b = G::angle(this, nb1, this, nb2);
        Angle c = G::angle(this, &circle, this, nb2);

        // if O is outside range of node, nb1 and nb2 create a stuck angle with node
        if (b >= M_PI || (fabs(a) + fabs(c) == fabs(b) && G::distance(this, circle) > range_))
        {
            stuckangle* new_angle = new stuckangle();
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
    if (b >= M_PI || (fabs(a) + fabs(c) == fabs(b) && G::distance(this, circle) > range_))
    {
        stuckangle* new_angle = new stuckangle();
        new_angle->a_ = nb1;
        new_angle->b_ = nb2;
        new_angle->next_ = stuck_angle_;
        stuck_angle_ = new_angle;
    }
}

void
GridOnlineAgent::sendBoundHole()
{
    Packet		*p;
    hdr_cmn		*cmh;
    hdr_ip		*iph;
    hdr_grid	*bhh;

    for (stuckangle * sa = stuck_angle_; sa; sa = sa->next_)
    {
        p = allocpkt();

        p->setdata(new GridOnlinePacketData()); // packet data = ?

        cmh = HDR_CMN(p);
        iph = HDR_IP(p);
        bhh = HDR_GRID(p);

        cmh->ptype() 	 = PT_GRID;
        cmh->direction() = hdr_cmn::DOWN;
        cmh->size() 	+= IP_HDR_LEN + bhh->size();
        cmh->next_hop_	 = sa->a_->id_;
        cmh->last_hop_ 	 = my_id_;
        cmh->addr_type_  = NS_AF_INET;

        iph->saddr() = my_id_;
        iph->daddr() = sa->a_->id_;
        iph->sport() = RT_PORT;
        iph->dport() = RT_PORT;
        iph->ttl_ 	 = limit_boundhole_hop_;			// more than ttl_ hop => boundary => remove

        bhh->prev_ = *this;
        bhh->last_ = *(sa->b_);
        bhh->i_ = *this;

        send(p, 0);

        printf("%d\t- Send GridOnline\n", my_id_);
    }
}

void
GridOnlineAgent::recvBoundHole(Packet *p)
{
    struct hdr_ip	*iph = HDR_IP(p);
    struct hdr_cmn 	*cmh = HDR_CMN(p);
    struct hdr_grid *bhh = HDR_GRID(p);

    // if the grid packet has came back to the initial node
    if (iph->saddr() == my_id_)
    {
        if (iph->ttl_ > (limit_boundhole_hop_ - 5))
        {
            drop(p, " SmallHole");	// drop hole that have less than 5 hop
        }
        else
        {
            addData(p, true);
            createPolygonHole(p);

            dumpBoundhole();
            dumpTime();
            dumpEnergy();
            dumpArea();

            drop(p, " GRID");
        }
        return;
    }

    if (iph->ttl_-- <= 0)
    {
        drop(p, DROP_RTR_TTL);
        return;
    }

    // add data to packet
    addData(p);


    // ------------ forward packet
    node* nb = getNeighborByBoundhole(&bhh->prev_, &bhh->last_);

    // no neighbor to forward, drop message. it means the network is not interconnected
    if (nb == NULL)
    {
        drop(p, DROP_RTR_NO_ROUTE);
        return;
    }

    // if neighbor already send grid message to that node
    if (iph->saddr() > my_id_)
    {
        for (stuckangle *sa = stuck_angle_; sa; sa = sa->next_)
        {
            if (sa->a_->id_ == nb->id_)
            {
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

node*
GridOnlineAgent::getNeighborByBoundhole(Point * p, Point * prev)
{
    Angle max_angle = -1;
    node* nb = NULL;

    for (node * temp = neighbor_list_; temp; temp = temp->next_)
    {
        Angle a = G::angle(this, p, this, temp);
        if (a > max_angle && (!G::is_intersect(this, temp, p, prev) ||
                              (temp->x_ == p->x_ && temp->y_ == p->y_) ||
                              (this->x_ == prev->x_ && this->y_ == prev->y_)))
        {
            max_angle = a;
            nb = temp;
        }
    }

    return nb;
}

// ------------------------ add data ------------------------ //

void
GridOnlineAgent::addData(Packet* p, bool isLast)
{
    struct hdr_cmn 	*cmh = HDR_CMN(p);
    struct hdr_grid *bhh = HDR_GRID(p);
    GridOnlinePacketData *data = (GridOnlinePacketData*)p->userdata();

    // data->dump();

    // Add data to packet
    if (cmh->num_forwards_ == 1)
    {
        if (fmod(bhh->i_.x_, r_) == 0)	// i lies in vertical line
        {
            if 		(this->x_ > bhh->i_.x_)	bhh->i_.x_ += r_ / 2;
            else if (this->x_ < bhh->i_.x_)	bhh->i_.x_ -= r_ / 2;
            else // (this->x_ == bhh->i_.x_)
            {
                if (this->y_ > bhh->i_.y_)	bhh->i_.x_ += r_ / 2;
                else 						bhh->i_.x_ -= r_ / 2;
            }
        }
        if (fmod(bhh->i_.y_, r_) == 0)	// i lies in h line
        {
            if 		(this->y_ > bhh->i_.y_)	bhh->i_.y_ += r_ / 2;
            else if (this->y_ < bhh->i_.y_)	bhh->i_.y_ -= r_ / 2;
            else // (this->y_ == bhh->i_.y_)
            {
                if (this->x_ > bhh->i_.x_)	bhh->i_.y_ -= r_ / 2;
                else 						bhh->i_.y_ += r_ / 2;
            }
        }

        bhh->i_.x_ = ((int)(bhh->i_.x_ / r_) + 0.5) * r_;
        bhh->i_.y_ = ((int)(bhh->i_.y_ / r_) + 0.5) * r_;
    }

    Point i[4];
    Line l = G::line(bhh->prev_, this);
    while ((fabs(this->x_ - bhh->i_.x_) > r_ / 2) || (fabs(this->y_ - bhh->i_.y_) > r_ / 2))
    {
        i[Up].x_ 	= bhh->i_.x_;		i[Up].y_ 	= bhh->i_.y_ + r_;
        i[Left].x_	= bhh->i_.x_ - r_;	i[Left].y_ 	= bhh->i_.y_;
        i[Down].x_	= bhh->i_.x_;		i[Down].y_ 	= bhh->i_.y_ - r_;
        i[Right].x_ = bhh->i_.x_ + r_;	i[Right].y_ = bhh->i_.y_;

        int m = this->x_ > bhh->prev_.x_ ? Right : Left;
        int n = this->y_ > bhh->prev_.y_ ? Up : Down;
        if (G::distance(i[m], l) > G::distance(i[n], l)) m = n;

        double dx, dy;
        switch (m)
        {
            case Up:	dx = - r_ / 2;	dy = + r_ / 2;	break;
            case Left:	dx = - r_ / 2;	dy = - r_ / 2;	break;
            case Down:	dx = + r_ / 2;	dy = - r_ / 2;	break;
            case Right:	dx = + r_ / 2;	dy = + r_ / 2;	break;
        }

        int lastcount = data->size();

        addMesh(data, bhh->i_.x_ + dx, bhh->i_.y_ + dy, isLast);

        if (!isLast) addMesh(data, i[m].x_ + dx, i[m].y_ + dy);

        cmh->size() += (data->size() - lastcount) * sizeof(Point);
        bhh->i_ = i[m];
    }
}

void GridOnlineAgent::addMesh(GridOnlinePacketData *data, double x, double y, bool isLast)
{
    Point p;
    p.x_ = x;
    p.y_ = y;

    // data->dump();

    // remove loop cycle
    for (int i = isLast; i < data->size(); i++)
    {
        Point u = data->getData(i);
        if (u == p)
        {
            while (data->size() > i) data->removeData(i);
        }
    }

    // data->dump();

    // add 45 degree missing node before p
    if (data->size() > 0)
    {
        Point u = data->getData(data->size() - 1);
        if (u.x_ != p.x_ && u.y_ != p.y_)
        {
            if ((u.x_ - p.x_) * (u.y_ - p.y_) > 0)
                addMesh(data, u.x_, p.y_);
            else
                addMesh(data, p.x_, u.y_);
        }
    }

    // data->dump();

    // remove preview node lies in same line
    if (data->size() > 1)
    {
        Point u = data->getData(data->size() - 1);
        Point v = data->getData(data->size() - 2);

        if (G::is_in_line(v, u, p))
            data->removeData(data->size() - 1);
    }

    // data->dump();

    // add data
    if (!isLast || !G::is_in_line(data->getData(data->size() - 1), p, data->getData(0)))
    {
        data->addData(p);
    }

    // data->dump();

    // add 45 degree missing node after p
    if (isLast)
    {
        Point u = data->getData(data->size() - 1);
        Point p = data->getData(0);
        if (u.x_ != p.x_ && u.y_ != p.y_)
        {
            if ((u.x_ - p.x_) * (u.y_ - p.y_) > 0)
                addMesh(data, u.x_, p.y_);
            else
                addMesh(data, p.x_, u.y_);
        }
    }

    // data->dump();

    // remove next node lies in same line
    if (isLast && G::is_in_line(data->getData(0), data->getData(1), data->getData(data->size() - 1)))
    {
        data->removeData(0);
    }

    // data->dump();

    // reduce hole
    while (data->size() > limit_ && limit_ >= 4)
    {
        // data->dump();

        int imin = -1;
        int min = MAXINT;
        Point r;
        for (int i = data->size() - 4; i > 0; i--)
        {
            Point g1 = data->getData(i);
            Point g2 = data->getData(i + 1);
            Point g3 = data->getData(i + 2);

            if (G::angle(g2, g1, g2, g3) < M_PI)
            {
                //	int t = fabs(g3.x_ - g2.x_) + fabs(g2.y_ - g1.y_) + fabs(g3.y_ - g2.y_) + fabs(g2.x_ - g1.x_);	// conditional is boundary length
                int t = fabs(g3.x_ - g2.x_) * fabs(g2.y_ - g1.y_) + fabs(g3.y_ - g2.y_) * fabs(g2.x_ - g1.x_);	// conditional is area
                if (t <= min)
                {
                    imin = i;
                    min  = t;
                    r.x_ = g1.x_ + g3.x_ - g2.x_;
                    r.y_ = g1.y_ + g3.y_ - g2.y_;
                }
            }
        }

        if (imin > -1)
        {
            data->removeData(imin);
            data->removeData(imin);
            data->removeData(imin);

            if (data->getData(imin) == r)
            {
                data->removeData(imin);
            }
            else
            {
                data->addData(imin, r);
            }

            // data->dump();
        }
        else
        {
            break;
        }
    }
}

// ------------------------ Create PolygonHole ------------------------ //

void
GridOnlineAgent::createPolygonHole(Packet* p)
{
    GridOnlinePacketData* data = (GridOnlinePacketData*)p->userdata();

    // create hole item
    polygonHole * hole_item = new polygonHole();
    hole_item->node_list_ 	= NULL;
    hole_item->next_ = hole_list_;
    hole_list_ = hole_item;

    // add node info to hole item
    struct node* item;

    for (int i = 0; i < data->size(); i++)
    {
        Point n = data->getData(i);

        item = new node();
        item->x_	= n.x_;
        item->y_	= n.y_;
        item->next_ = hole_item->node_list_;
        hole_item->node_list_ = item;
    }
}

// ------------------------ Dump ------------------------ //

void
GridOnlineAgent::dumpTime()
{
    FILE * fp = fopen("Time.tr", "a+");
    fprintf(fp, "%f\n", Scheduler::instance().clock());
    fclose(fp);
}

void
GridOnlineAgent::dumpBoundhole()
{
    FILE *fp = fopen("GridOnline.tr", "a+");

    for (polygonHole* p = hole_list_; p != NULL; p = p->next_)
    {
        node* n = p->node_list_;
        do {
            fprintf(fp, "%f	%f\n", n->x_, n->y_);
            n = n->next_;
        } while (n && n != p->node_list_);

        fprintf(fp, "%f	%f\n\n", p->node_list_->x_, p->node_list_->y_);
    }

    fclose(fp);
}

void
GridOnlineAgent::dumpArea()
{
    if (hole_list_)
    {
        FILE * fp = fopen("Area.tr", "a+");
        fprintf(fp, "%f\n", G::area(hole_list_->node_list_));
        fclose(fp);
    }
}