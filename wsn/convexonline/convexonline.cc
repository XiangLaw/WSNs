/*
 * convex.cc
 *
 *  Last edited on Nov 14, 2013
 *  by Trong Nguyen
 */

#include "config.h"
#include "convexonline.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#include "../include/tcl.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static class ConvexOnlineAgentClass : public TclClass
{
	public:
		ConvexOnlineAgentClass() : TclClass("Agent/CONVEXONLINE") {}
		TclObject* create(int, const char*const*)
		{
			return (new ConvexOnlineAgent());
		}
}class_convex;

void
ConvexOnlineTimer::expire(Event *e) {
	(a_->*firing_)();
}

// ------------------------ Agent ------------------------ //

ConvexOnlineAgent::ConvexOnlineAgent() : GPSRAgent(),
		findStuck_timer_(this, &ConvexOnlineAgent::findStuckAngle),
		convex_timer_(this, &ConvexOnlineAgent::sendBoundHole)
{
	stuck_angle_ = NULL;
	hole_list_ = NULL;
	bind("range_", &range_);
	bind("limit_", &limit_);
	bind("r_", &r_);
	bind("limit_boundhole_hop_", &limit_boundhole_hop_);
}

int
ConvexOnlineAgent::command(int argc, const char*const* argv)
{
	if (argc == 2)
	{
		if (strcasecmp(argv[1], "start") == 0)
		{
			startUp();
		}
		if (strcasecmp(argv[1], "boundhole") == 0)
		{
			convex_timer_.resched(randSend_.uniform(0.0, 5));
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
ConvexOnlineAgent::recv(Packet *p, Handler *h)
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

//		case PT_CBR:
//			if (iph->saddr() == my_id_)				// a packet generated by myself
//			{
//				if (cmh->num_forwards() == 0)		// a new packet
//				{
//					sendData(p);
//				}
//				else	//(cmh->num_forwards() > 0)	// routing loop -> drop
//				{
//					drop(p, DROP_RTR_ROUTE_LOOP);
//					return;
//				}
//			}
//
//			if (iph->ttl_-- <= 0)
//			{
//				drop(p, DROP_RTR_TTL);
//				return;
//			}
//			recvData(p);
//			break;

		default:
			drop(p, " UnknowType");
			break;
	}
}

void
ConvexOnlineAgent::startUp()
{
	findStuck_timer_.resched(20);

	// clear trace file
	FILE *fp;
	fp = fopen("Area.tr", 		"w");		fclose(fp);
	fp = fopen("ConvexOnline.tr", 	"w");	fclose(fp);
	fp = fopen("Neighbors.tr", 	"w");		fclose(fp);
	fp = fopen("Time.tr", 		"w");		fclose(fp);
}

// ------------------------ Bound hole ------------------------ //

void
ConvexOnlineAgent::findStuckAngle()
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
ConvexOnlineAgent::sendBoundHole()
{
	Packet		*p;
	hdr_cmn		*cmh;
	hdr_ip		*iph;
	hdr_grid	*bhh;

	for (stuckangle * sa = stuck_angle_; sa; sa = sa->next_)
	{
		p = allocpkt();

		p->setdata(new GridOnlinePacketData());

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

		printf("%d\t- Send ConvexOnline\n", my_id_);
	}
}

void
ConvexOnlineAgent::recvBoundHole(Packet *p)
{
	struct hdr_ip	*iph = HDR_IP(p);
	struct hdr_cmn 	*cmh = HDR_CMN(p);
	struct hdr_grid *bhh = HDR_GRID(p);

	// if the convex packet has came back to the initial node
	if (iph->saddr() == my_id_)
	{
		if (iph->ttl_ > (limit_boundhole_hop_ - 5))
		{
			drop(p, " SmallHole");	// drop hole that have less than 5 hop
		}
		else
		{
			createConvexHole(p);

			dumpBoundhole();
			dumpTime();
			dumpEnergy();
			dumpArea();

			drop(p, " BOUNDHOLE");
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

	// if neighbor already send convex message to that node
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
ConvexOnlineAgent::getNeighborByBoundhole(Point * p, Point * prev)
{
	Angle max_angle = -1;
	node* nb = NULL;

	for (node * temp = neighbor_list_; temp; temp = temp->next_)
	{
		Angle a = G::angle(this, p, this, temp);
		if (a > max_angle && !G::is_intersect(this, temp, p, prev))
		{
			max_angle = a;
			nb = temp;
		}
	}

	return nb;
}

// ------------------------ add data ------------------------ //

void
ConvexOnlineAgent::addData(Packet* p)
{
	struct hdr_cmn 	*cmh = HDR_CMN(p);
	GridOnlinePacketData *data = (GridOnlinePacketData*)p->userdata();

	int lastcount = data->size();

	// remove preview node lies inside convex
	while (data->size() > 1 && G::angle(data->getData(data->size() - 2), data->getData(data->size() - 1)) < G::angle(data->getData(data->size() - 2), this))
	{
		data->removeData(data->size() - 1);
	}

	// add data
	data->addData(*this);

	// reduce hole
	while (data->size() > limit_ && limit_ >= 4)
	{
		double min = DBL_MAX;
		int imin = -1;

		for (int i = data->size() - 4; i >= 0; i--)
		{
			Point insp;
			if (G::intersection(G::line(data->getData(i), data->getData(i + 1)), G::line(data->getData(i + 2), data->getData(i + 3)), insp))
			{
				double s = G::area(data->getData(i + 1), insp, data->getData(i + 2));
				if (s < min)
				{
					min = s;
					imin = i;
				}
			}
		}

		if (imin > -1)
		{
			// repeat 2 old nodes by insp node
			Point insp;
			G::intersection(G::line(data->getData(imin), data->getData(imin + 1)), G::line(data->getData(imin + 2), data->getData(imin + 3)), insp);

			// remove 2 old nodes
			data->removeData(imin + 1);
			data->removeData(imin + 1);

			data->addData(imin + 1, insp);
		}
		else
		{
			break;
		}
	}

	cmh->size() += (data->size() - lastcount) * sizeof(Point);
}

// ------------------------ Create PolygonHole ------------------------ //

polygonHole*
ConvexOnlineAgent::createPolygonHole(Packet* p)
{
	GridOnlinePacketData* data = (GridOnlinePacketData*)p->userdata();

	// create hole item
	polygonHole * hole_item = new polygonHole();
	hole_item->node_list_ 	= NULL;

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

	return hole_item;
}

/*
 * Find convex hull
 */
void
ConvexOnlineAgent::createConvexHole(Packet* p)
{
	polygonHole* h = createPolygonHole(p);

	polygonHole * newHole	= new polygonHole();
	newHole->hole_id_ 		= h->hole_id_;
	newHole->node_list_		= NULL;
	newHole->next_ 			= hole_list_;
	hole_list_ = newHole;

	/*
	 * define 4 line for new approximate hole
	 *
	 * 	  __________[2]____________
	 * 	 |		 				   |
	 * 	[0]--------base line------[1]
	 * 	 |______________   ________|
	 * 					[3]
	 */

	Line bl;			// base line

	node* n0 = NULL;
	node* n1 = NULL;
	node* n2 = NULL;
	node* n3 = NULL;

	// find couple node that have maximum distance - n0, n1
	double mdis = 0;
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

	// cycle h->node_list_
	node* temp = h->node_list_;
	while (temp->next_) temp = temp->next_;
	temp->next_ = h->node_list_;

	// find n2 and n3 - with maximum distance from base line
	mdis = 0;
	for (struct node* i = n0; i != n1->next_; i = i->next_)
	{
		double dis = G::distance(i, bl);
		if (dis > mdis)
		{
			mdis = dis;
			n2 = i;
		}
	}
	mdis = 0;
	for (struct node* i = n1; i != n0->next_; i = i->next_)
	{
		double dis = G::distance(i, bl);
		if (dis > mdis)
		{
			mdis = dis;
			n3 = i;
		}
	}

	// check if n2 and n3 in other side of base line or not
	if (G::position(n2, bl) * G::position(n3, bl) > 0) // n2 and n3 in same side of base line
	{
		if (G::distance(n2, bl) > G::distance(n3, bl))
		{
			// re-find n3 in other side of bl
			double n2side = G::position(n2, bl);
			mdis = 0;
			for (struct node* i = n1; i != n0->next_; i = i->next_)
			{
				if (G::position(i, bl) * n2side <= 0)
				{
					double dis = G::distance(i, bl);
					if (dis > mdis)
					{
						mdis = dis;
						n3 = i;
					}
				}
			}
		}
		else	// re-find n2 in other side of bl
		{
			double n3side = G::position(n3, bl);
			mdis = 0;
			for (struct node* i = n0; i != n1->next_; i = i->next_)
			{
				if (G::position(i, bl) * n3side <= 0)
				{
					double dis = G::distance(i, bl);
					if (dis > mdis)
					{
						mdis = dis;
						n2 = i;
					}
				}
			}
		}
	}

	// ------------ calculate approximate
	addHoleNode(newHole, n0);	findEx(newHole, n0, n2, n1);
	addHoleNode(newHole, n2);	findEx(newHole, n2, n1, n0);
	addHoleNode(newHole, n1);	findEx(newHole, n1, n3, n0);
	addHoleNode(newHole, n3);	findEx(newHole, n3, n0, n1);

	// cycle the node_list
	int count = 1;
	temp = newHole->node_list_;
	while (temp->next_)
	{
		temp = temp->next_;
		count++;
	}
	temp->next_ = newHole->node_list_;

	// free h
	delete h;

	// reduce
	if (limit_ > 0)
	{
		int conti = true;
		for (int i = count; i > limit_ && conti; i--)
		{
			conti = reduce(newHole);
		}
	}
}

bool ConvexOnlineAgent::reduce(polygonHole* h)
{
	double min = DBL_MAX;
	node* re = NULL;

	node* i = h->node_list_;
	bool ok = false;
	do {
		Point insp;
		if (G::intersection(G::line(i, i->next_), G::line(i->next_->next_, i->next_->next_->next_), insp))
		{
			double dis = G::distance(i->next_, insp) + G::distance(i->next_->next_, insp) - G::distance(i->next_, i->next_->next_);
			if (dis < min)
			{
				min = dis;
				re = i;
			}
			ok = true;
		}
		i = i->next_;
	} while (i != h->node_list_ && ok);

	if (ok)
	{
		// repeat 2 old nodes by insp node
		node* newNode = new node();
		G::intersection(G::line(re, re->next_), G::line(re->next_->next_, re->next_->next_->next_), newNode);
		newNode->next_ = re->next_->next_->next_;

		delete re->next_;			// remove 2 old nodes
		delete re->next_->next_;

		re->next_ = newNode;
		h->node_list_ = re;
	}

	return ok;
}

void ConvexOnlineAgent::findEx(polygonHole* h, node* n1, node* n2, node* p)
{
	Line l = G::line(n1, n2);
	int pside = G::position(p, l);
	double max = 0;
	node* n = NULL;

	for	(node* i = n1->next_; i != n2; i = i->next_)
	{
		if (G::position(i, l) != pside)
		{
			double dis = G::distance(i,l);
			if (dis > max)
			{
				max = dis;
				n = i;
			}
		}
	}

	if (n)
	{
		findEx(h, n1, n, p);
		addHoleNode(h, n);
		findEx(h, n, n2, p);
	}
}

void ConvexOnlineAgent::addHoleNode(polygonHole* h, node* orginNode)
{
	for (node* i = h->node_list_; i; i = i->next_)
	{
		if (*i == *orginNode) return;
	}

	node * newNode	= new node();
	newNode->x_ 	= orginNode->x_;
	newNode->y_ 	= orginNode->y_;
	newNode->next_ 	= h->node_list_;
	h->node_list_ = newNode;
}

// ------------------------ Dump ------------------------ //

void
ConvexOnlineAgent::dumpTime()
{
	FILE * fp = fopen("Time.tr", "a+");
	fprintf(fp, "%f\n", Scheduler::instance().clock());
	fclose(fp);
}

void
ConvexOnlineAgent::dumpBoundhole()
{
	FILE *fp = fopen("ConvexOnline.tr", "a+");

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
ConvexOnlineAgent::dumpArea()
{
	if (hole_list_)
	{
		FILE * fp = fopen("Area.tr", "a+");
		fprintf(fp, "%f\n", G::area(hole_list_->node_list_));
		fclose(fp);
	}
}
