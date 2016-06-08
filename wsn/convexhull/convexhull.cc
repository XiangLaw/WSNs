/*
 * convexhull.cc
 *
 *  Last edited on Nov 14, 2013
 *  by Trong Nguyen
 */


#include "convexhull.h"

int hdr_convexhull::offset_;
Point ConvexHullAgent::NullPoint;

static class ConvexHullHeaderClass : public PacketHeaderClass {
public:
	ConvexHullHeaderClass() : PacketHeaderClass("PacketHeader/CONVEXHULL", sizeof(hdr_convexhull))
	{
		bind_offset(&hdr_convexhull::offset_);
	}
} class_convexhullhdr;

// --------------------------- Agent ------------------------------ //

ConvexHullAgent::ConvexHullAgent() : BoundHoleAgent()
{
	NullPoint.x_ = -1;
	NullPoint.y_ = -1;

	hole_list_ = NULL;

	bind("limit_", &limit_);

	// clear trace file
	FILE * fp;

	fp = fopen("ApproximateHole.tr", "w");	fclose(fp);
	fp = fopen("BroadcastRegion.tr", "w");	fclose(fp);
	fp = fopen("Time.tr", "w");				fclose(fp);
	fp = fopen("Area.tr", "w");				fclose(fp);
}

void ConvexHullAgent::recv(Packet* p, Handler* h)
{
	hdr_cmn * cmh = HDR_CMN(p);

	if (cmh->ptype() == PT_CONVEXHULL)
	{
		recvBCH(p);
	}
	else
	{
		BoundHoleAgent::recv(p, h);
	}
}

int
ConvexHullAgent::command(int argc, const char*const* argv)
{
	return BoundHoleAgent::command(argc,argv);
}

// --------------------- Approximate hole ------------------------- //

/*
 * Find convex hull
 */
void
ConvexHullAgent::createHole(Packet* p)
{
	polygonHole* h = createPolygonHole(p);

	//dumpArea(h, "OriginArea.tr");

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

	// dump
	dumpApproximateHole();
	dumpTime();
	//dumpEnergy();
	//dumpArea(newHole, "Area.tr");

	// Broadcast hole information
	sendBCH(newHole);
}

void ConvexHullAgent::findEx(polygonHole* h, node* n1, node* n2, node* p)
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

void ConvexHullAgent::addHoleNode(polygonHole* h, node* orginNode)
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

bool ConvexHullAgent::reduce(polygonHole* h)
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

// --------------------- Routing ---------------------------------- //

//Point ConvexHullAgent::routing(Point* dest)
void ConvexHullAgent::routing(hdr_convexhull* edh)
{
	Point* dest = &(edh->des);

	// check if h is intersect with l
	bool isIntersect = false;
	if (hole_list_)
	{
		node* n = hole_list_->node_list_;
		do {
			isIntersect = G::is_intersect2(this, dest, n, n->next_);
			n = n->next_;
		} while (n != hole_list_->node_list_ && !isIntersect);
	}
	if (isIntersect)
	{
		// create routing table for packet p
		node* S1;	// min angle view of this node to hole
		node* S2;	// max angle view of this node to hole
		node* D1;	// min angle view of Dt node to hole
		node* D2;	// max angle view of Dt node to hole
		node* re;	// result point

		// ------------------- S1 S2 - view angle of this node to hole
		double Smax = 0;
		double Dmax = 0;

		node* i = hole_list_->node_list_;
		do {
			for (node* j = i->next_; j != hole_list_->node_list_; j = j->next_)
			{
				// S1 S2
				double angle = G::angle(this, i, j);
				if (angle > Smax)
				{
					Smax = angle;
					S1 = i;
					S2 = j;
				}
				// D1 D2
				angle = G::angle(dest, i, j);
				if (angle > Dmax)
				{
					Dmax = angle;
					D1 = i;
					D2 = j;
				}
			}
			i = i->next_;
		} while(i != hole_list_->node_list_);

		// if S1 and D1 are lie in different side of SD => switch D1 and D2
		Line SD = G::line(this, dest);
		if (G::position(S1, SD) != G::position(D1, SD))
		{
			node* temp = D1;
			D1 = D2;
			D2 = temp;
		}

		double min = DBL_MAX;
		// ------------------------------------------------- S S1 D1 D
		double dis = G::distance(this, S1);
		for (node * i = S1; i != D1; i = i-> next_)
		{
			dis += G::distance(i, i->next_);
		}
		dis += G::distance(D1, dest);
		if (dis < min)
		{
			min = dis;
			re = S1;
		}

		// ------------------------------------------------- S S2 D2 D
		dis = G::distance(this, S2);
		for (node * i = S2; i != D2; i = i->next_)
		{
			dis += G::distance(i, i->next_);
		}
		dis += G::distance(D2, dest);
		if (dis < min)
		{
			min = dis;
			re = S2;
		}

		// ------------------------------------------------- D D1 S1 S
		dis = G::distance(dest, D1);
		for (node * i = D1; i != S1; i = i->next_)
		{
			dis += G::distance(i, i->next_);
		}
		dis += G::distance(S1, this);
		if (dis < min)
		{
			min = dis;
			re = S1;
		}

		// ------------------------------------------------- D D2 S2 S
		dis = G::distance(dest, D2);
		for (node * i = D2; i != S2; i = i->next_)
		{
			dis += G::distance(i, i->next_);
		}
		dis += G::distance(S2, this);
		if (dis < min)
		{
			min = dis;
			re = S2;
		}

		edh->sub = *re;

		return;
	}

	edh->sub = *dest;
}

// --------------------- Send data -------------------------------- //

void ConvexHullAgent::sendData(Packet* p)
{
	hdr_cmn*		cmh = HDR_CMN(p);
	hdr_ip*			iph = HDR_IP(p);
	hdr_convexhull* edh = HDR_CONCVEXHULL(p);

	cmh->size() += IP_HDR_LEN + edh->size();
	cmh->direction() = hdr_cmn::DOWN;

	edh->daddr_ = iph->daddr();
	edh->des = *dest;
	edh->sub = NullPoint;

	iph->saddr() = my_id_;
	iph->daddr() = -1;
	iph->ttl_ = 100;
}

void ConvexHullAgent::recvData(Packet* p)
{
	struct hdr_cmn*			cmh = HDR_CMN(p);
	struct hdr_convexhull* 	edh = HDR_CONCVEXHULL(p);

	if (cmh->direction() == hdr_cmn::UP	&& edh->daddr_ == my_id_)	// up to destination
	{
		port_dmux_->recv(p, 0);
		return;
	}
	else
	{
		// -------- have hole's information
		if (edh->sub == NullPoint && hole_list_)
		{
			//edh->sub = routing(&(edh->des));
			routing(edh);
		}

		// -------- read sub-destination
		if (edh->sub == *this)
		{
			//edh->sub = routing(&(edh->des));
			routing(edh);
		}

		// -------- forward by greedy
		node * next = getNeighborByGreedy(edh->sub == NullPoint ? edh->des : edh->sub);

		if (next == NULL)	// no neighbor close
		{
			drop(p, "Stuck");
			return;
		}
		else
		{
			cmh->direction() = hdr_cmn::DOWN;
			cmh->addr_type() = NS_AF_INET;
			cmh->last_hop_ = my_id_;
			cmh->next_hop_ = next->id_;
			send(p, 0);
		}
	}
}

// --------------------- dump ------------------------------------- //

void ConvexHullAgent::dumpApproximateHole()
{
	FILE *fp = fopen("ApproximateHole.tr", "a+");

	node* n = hole_list_->node_list_;
	do {
		fprintf(fp, "%f	%f\n", n->x_, n->y_);
		n = n->next_;
	} while (n != hole_list_->node_list_);
	fprintf(fp, "%f	%f\n\n", hole_list_->node_list_->x_, hole_list_->node_list_->y_);

	fclose(fp);
}

void
ConvexHullAgent::dumpArea(polygonHole* h, char* fileName)
{
	FILE * fp = fopen(fileName, "w");
	fprintf(fp, "%f\n", G::area(h->node_list_));
	fclose(fp);
}

void
ConvexHullAgent::dumpTime()
{
	FILE * fp = fopen("Time.tr", "a+");
	fprintf(fp, "%f\n", Scheduler::instance().clock());
	fclose(fp);
}

// ------------------------ End ------------------------ //
