/*
 * ellipse.cc
 *
 * Created on: Jan 6, 2014
 * author :    trongnguyen
 */

#include "ellipse.h"
#include "../include/tcl.h"

int hdr_ellipse::offset_;

static class EllipseHeaderClass : public PacketHeaderClass
{
public:
	EllipseHeaderClass() : PacketHeaderClass("PacketHeader/ELLIPSE", sizeof(hdr_all_ellipse)) {
		bind_offset(&hdr_ellipse::offset_);
	}
}class_ellipsehdr;

static class EllipseAgentClass : public TclClass
{
public:
	EllipseAgentClass() : TclClass("Agent/ELLIPSE") {}
	TclObject* create(int, const char*const*)
	{
		return (new EllipseAgent());
	}
}class_ellipse;

// ------------------------ Agent ------------------------ //

EllipseAgent::EllipseAgent() : BoundHoleAgent()
{
	ellipse_list_ = NULL;

	bind("alpha_", &alpha_);

	FILE * fp;
	fp = fopen("ApproximateHole.tr", "w");	fclose(fp);
	fp = fopen("Hopcount.tr", "w"); 		fclose(fp);
	fp = fopen("BroadcastRegion.tr", "w"); 	fclose(fp);
	fp = fopen("RoutingTable.tr", "w");		fclose(fp);
}

void EllipseAgent::recv(Packet *p, Handler *h)
{
	hdr_cmn *cmh = HDR_CMN(p);

	if (cmh->ptype() == PT_ELLIPSE)
	{
		recvBroadcast(p);
	}
	else
	{
		BoundHoleAgent::recv(p, h);
	}
}

int EllipseAgent::command(int argc, const char*const* argv)
{
	if (argc == 2)
	{
		if (strcasecmp(argv[1], "dump") == 0)
		{

		}
	}

	return BoundHoleAgent::command(argc, argv);
}

// ------------------------ Approximate hole ------------------------ //

void EllipseAgent::createHole(Packet* p)
{
	polygonHole* h = createPolygonHole(p);

	// create new ellipse hole
	ellipseHole * newHole 	= new ellipseHole();
	newHole->hole_id_		= h->hole_id_;
	newHole->next_ 			= ellipse_list_;
	ellipse_list_ 			= newHole;

	node* n0 = NULL;
	node* n1 = NULL;
	node* n2 = NULL;
	node* n3 = NULL;

	Line bl;			// base line

	double mdis = 0;

	// find couple node that have maximum distance - n0, n1
	for	(struct node* i = h->node_list_; i; i = i->next_)
	{
		for (struct node* j = i->next_; j; j = j->next_)
		{
			double dis = G::distance(i, j);
			if (mdis < dis)
			{
				mdis = dis;
				n0 = i;
				n1 = j;
			}
		}
	}

	bl = G::line(n0, n1);

	// find n2 - with maximum distance from base line
	mdis = 0;
	for (struct node* i = h->node_list_; i; i = i->next_)
	{
		double dis = G::distance(i, bl);
		if (dis > mdis)
		{
			mdis = dis;
			n2 = i;
		}
	}
	int n2side = G::position(n2, bl);

	// find n3 - in other side with n2 and maximum distance from base line
	mdis = 0;
	for (struct node* i = h->node_list_; i; i = i->next_)
	{
		if (G::position(i, bl) != n2side)
		{
			double dis = G::distance(i, bl);
			if (dis > mdis)
			{
				mdis = dis;
				n3 = i;
			}
		}
	}

	Line l0 = G::perpendicular_line(n0, bl);	// line contain n0 and perpendicular with base line
	Line l1 = G::perpendicular_line(n1, bl);	// line contain n1 and perpendicular with base line
	Line l2 = G::parallel_line(n2, bl);			// line contain n2 and parallel with base line
	Line l3 = G::parallel_line(n3, bl);			// line contain n2 and parallel with base line

	// four corner of rectangle
	Point p0, p1, p2, p3;
	G::intersection(l0, l2, p0);
	G::intersection(l0, l3, p1);
	G::intersection(l1, l3, p2);
	G::intersection(l1, l2, p3);

	// for bisector of rectangle
	Line bis0 = G::angle_bisector(p0, p1, p3);
	Line bis1 = G::angle_bisector(p1, p2, p0);
	Line bis2 = G::angle_bisector(p2, p1, p3);
	Line bis3 = G::angle_bisector(p3, p2, p0);

	// find f2
	G::intersection(bis0, bis1, newHole->f1_);
	G::intersection(bis2, bis3, newHole->f2_);

	// find a
	newHole->a_ = 0;
	for (struct node* i = h->node_list_; i; i = i->next_)
	{
		double dis = G::distance(i, newHole->f1_) + G::distance(i, newHole->f2_);
		if (dis > newHole->a_) newHole->a_ = dis;
	}
	newHole->a_ = newHole->a_ / 2;

	// dump
	dumpApproximateHole();

	// broadcast hole information
	sendBroadcast(newHole);
}

// ------------------------ Broadcast hole information ------------------------ //

void EllipseAgent::sendBroadcast(ellipseHole* h)
{
	Packet*	p = allocpkt();
	struct hdr_cmn*					cmh = HDR_CMN(p);
	struct hdr_ip*					iph = HDR_IP(p);
	struct hdr_ellipse_broadcast*	ebh = HDR_ELLIPSE_BROADCAST(p);

	cmh->ptype() 	 = PT_ELLIPSE;
	cmh->next_hop_	 = IP_BROADCAST;
	cmh->last_hop_ 	 = my_id_;
	cmh->addr_type_  = NS_AF_INET;
	cmh->size()		 = IP_HDR_LEN + ebh->size();

	iph->daddr() = IP_BROADCAST;
	iph->saddr() = my_id_;
	iph->sport() = RT_PORT;
	iph->dport() = RT_PORT;
	iph->ttl_	 = IP_DEF_TTL;

	ebh->hole_id_ = h->hole_id_;
	ebh->F1 = h->f1_;
	ebh->F2 = h->f2_;
	ebh->a_ = h->a_;

	send(p, 0);
}

void EllipseAgent::recvBroadcast(Packet* p)
{
	// receive Ellipse packet
	struct hdr_ellipse_broadcast*	ebh = HDR_ELLIPSE_BROADCAST(p);
	ellipseHole* newHole;

	// check if is really receive this hole's information
	bool isReceived = false;
	for (ellipseHole* hole = ellipse_list_; hole && !isReceived; hole = hole->next_)
	{
		isReceived = hole->hole_id_ == ebh->hole_id_;
	}

	if (isReceived)
	{
		drop(p,"received");
	}
	else
	{
		// add new circle hole
		newHole 			= new ellipseHole();
		newHole->hole_id_	= ebh->hole_id_;
		newHole->f1_		= ebh->F1;
		newHole->f2_		= ebh->F2;
		newHole->a_			= ebh->a_;
		newHole->next_		= ellipse_list_;
		ellipse_list_		= newHole;

		// broadcast hole's information.
		if (G::distance(this, newHole->f1_) + G::distance(this, newHole->f2_) < newHole->a_ * 2)
		{
			hdr_cmn* cmh = HDR_CMN(p);

			cmh->direction() = hdr_cmn::DOWN;
			cmh->last_hop_   = my_id_;
			send(p, 0);
		}
		else
		{
			drop(p, "limited");
		}
	}

	dumpBroadcast();
}

// ------------------------ Routing ------------------------ //

void EllipseAgent::routing(Packet* p)
{
	hdr_ellipse_data * edh = HDR_ELLIPSE_DATA(p);

	Point p1, p2;
	Line ud = G::line(this, edh->des_);

	for (ellipseHole * e = ellipse_list_; e; e = e->next_)
	{
		if (e->hole_id_ != edh->hole_id && G::intersection(e, ud, p1, p2))
		{
			double	l = sqrt(e->a() * e->b()) * (1 + alpha_) - G::distance(G::midpoint(e->f1_, e->f2_), ud);
			Point	u = G::distance(this, p1) > G::distance(this, p2) ? p2 : p1;
			Line 	t = G::tangent(e, u);

			// find 2 points in t that have distance to u = L
			if (t.a_ == 0)
			{
				p1.y_ = p2.y_ = u.y_;
				p1.x_ = u.x_ + l;
				p2.x_ = u.x_ - l;
			}
			else
			{
				double a = t.b_ * t.b_ / (t.a_ * t.a_) + 1;
				double b = 2 * t.b_ / t.a_ * (t.c_ / t.a_ + u.x_) - 2 * u.y_;
				double c = (t.c_ / t.a_ + u.x_) * (t.c_ / t.a_ + u.x_) + u.y_ * u.y_ - l * l;

				G::quadratic_equation(a, b, c, p1.y_, p2.y_);
				p1.x_ = - (t.b_ * p1.y_ + t.c_) / t.a_;
				p2.x_ = - (t.b_ * p2.y_ + t.c_) / t.a_;
			}

			edh->sub_ = G::distance(p1, edh->des_) > G::distance(p2, edh->des_) ? p2 : p1;
			break;
		}
	}

	//free(ud);

	// dump
	dumpRoutingTable(edh);
}

// ------------------------ Send data ------------------------ //

void EllipseAgent::sendData(Packet * p)
{
	// send Data packet
	hdr_cmn*			cmh = HDR_CMN(p);
	hdr_ip*				iph = HDR_IP(p);
	hdr_ellipse_data* 	edh = HDR_ELLIPSE_DATA(p);

	cmh->size() += IP_HDR_LEN + edh->size();
	cmh->direction() = hdr_cmn::DOWN;
	cmh->addr_type() = NS_AF_INET;

	edh->daddr_	= iph->daddr();
	edh->sou_   = *(this);
	edh->des_	= *(this->dest);
	edh->sub_	= *(this->dest);

	iph->saddr() = my_id_;
	iph->daddr() = -1;
	iph->ttl_ 	 = 100;
}

void EllipseAgent::recvData(Packet * p)
{
	// receive Data packet
	struct hdr_cmn*				cmh = HDR_CMN(p);
	struct hdr_ellipse_data* 	edh = HDR_ELLIPSE_DATA(p);

	if (cmh->direction() == hdr_cmn::UP	&& edh->daddr_ == my_id_)	// up to destination
	{
		port_dmux_->recv(p, 0);
		return;
	}
	else
	{
		// meet hole
		if (ellipse_list_ && edh->sub_ == edh->des_)
		{
			routing(p);
		}

		// -------- forward by greedy

		node* nexthop = getNeighborByGreedy(edh->sub_);

		if (nexthop == NULL && edh->sub_ != edh->des_)	// reach sub-destination
		{
			edh->sub_ = edh->des_;
			nexthop = getNeighborByGreedy(edh->sub_);
		}

		if (nexthop == NULL)
		{
			drop(p, "Stuck");		// no neighbor close
		}
		else
		{
			cmh->direction() = hdr_cmn::DOWN;
			cmh->last_hop_ = my_id_;
			cmh->next_hop_ = nexthop->id_;
			send(p, 0);
		}
	}
}

// ------------------------ Dump ------------------------ //

void EllipseAgent::dumpApproximateHole()
{
	// TODO: dump approximate hole
}

void EllipseAgent::dumpBroadcast()
{
	FILE *fp = fopen("BroadcastRegion.tr", "a+");

	fprintf(fp, "%f	%f\n", this->x_, this->y_);

	fclose(fp);
}

void EllipseAgent::dumpRoutingTable(hdr_ellipse_data* hed)
{
	FILE* fp = fopen("RoutingTable.tr", "a+");
	fprintf(fp, "%d\t%f\t%f\n", hed->daddr_, hed->sou_.x_, hed->sou_.y_);
	fprintf(fp, "%d\t%f\t%f\n", hed->daddr_, hed->sub_.x_, hed->sub_.y_);
	fprintf(fp, "%d\t%f\t%f\n", hed->daddr_, hed->des_.x_, hed->des_.y_);
	fprintf(fp, "\n");

	fclose(fp);
}

// ------------------------ End ------------------------ //
