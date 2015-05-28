/*
 * scalegoal.cc
 *
 *  Last edited on Nov 14, 2013
 *  by Trong Nguyen
 */

#include "scalegoal.h"

int hdr_scalegoal::offset_;

static class ScaleGoalHeaderClass : public PacketHeaderClass {
public:
	ScaleGoalHeaderClass() : PacketHeaderClass("PacketHeader/SCALEGOAL", sizeof(hdr_scalegoal))
	{
		bind_offset(&hdr_scalegoal::offset_);
	}
} class_goalhdr;

static class ScaleGoalAgentClass : public TclClass
{
	public:
		ScaleGoalAgentClass() : TclClass("Agent/SCALEGOAL") {}
		TclObject* create(int, const char*const*)
		{
			return (new ScaleGoalAgent());
		}
}class_scalegoal;

// ------------------------ Agent ------------------------ //

ScaleGoalAgent::ScaleGoalAgent() : ConvexHullAgent()
{
	level_ = -1;
	distance_ = -1;
	bind("alpha_", &alpha_);
}

int ScaleGoalAgent::command(int argc, const char*const* argv)
{
	if (argc == 2)
	{
		if (strcasecmp(argv[1], "dump") == 0)
		{
			dumpBroadcast();
		}
	}

	return ConvexHullAgent::command(argc, argv);
}

// --------------------- Broadcast hole identifier ---------------- //

void ScaleGoalAgent::sendBCH(polygonHole* h)
{
	if (this->level_ == -1) this->level_ = 0;

	// ----------------- header
	Packet*	p = allocpkt();
	hdr_cmn*		cmh = HDR_CMN(p);
	hdr_ip*			iph = HDR_IP(p);
	hdr_scalegoal* 	sgh = HDR_SCALEGOAL(p);

	cmh->ptype() 	 = PT_CONVEXHULL;
	cmh->next_hop_	 = IP_BROADCAST;
	cmh->last_hop_ 	 = my_id_;
	cmh->addr_type_  = NS_AF_INET;
	cmh->size()		 = IP_HDR_LEN + sgh->size();

	iph->daddr() = IP_BROADCAST;
	iph->saddr() = my_id_;
	iph->sport() = RT_PORT;
	iph->dport() = RT_PORT;
	iph->ttl_	 = 100;

	sgh->hole_id_ 	= h->hole_id_;
	sgh->level_ 	= this->level_;
	sgh->g = maxEdge(h) / (pow(alpha_, (double)1 / (double)(level_ + 1)) - 1) * (1 / cos(2 * M_PI / numNode(h)) - 1);

	// ------------------ data
	BoundHolePacketData* data = new BoundHolePacketData();
	node* i = h->node_list_;
	do {
		data->add(h->hole_id_, i->x_, i->y_);
		i = i->next_;
	} while(i && i != h->node_list_);
	p->setdata(data);
	send(p, 0);

	//printf("%d (%f - %f) - send BCH %d\n", my_id_, x_, y_, cmh->uid_);
}

void ScaleGoalAgent::recvBCH(Packet* p)
{
	hdr_cmn*		cmh = HDR_CMN(p);
	hdr_scalegoal* 	sgh = HDR_SCALEGOAL(p);

	// check if is really receive this hole's information
	if (this->level_ >= 0)
	{
		if (hole_list_->hole_id_ == sgh->hole_id_)	// already received
		{
			if (this->level_ <= sgh->level_)
			{
				drop(p, "Received");
				return;
			}
			else
			{
				delete hole_list_;
				hole_list_ = NULL;
			}
		}
	}

	// update level
	this->level_ = sgh->level_;

	// create hole item
	polygonHole* newHole = createPolygonHole(p);
	newHole->hole_id_	 = sgh->hole_id_;
	newHole->next_ = NULL;

	// cycle the node_list
	newHole->circleNodeList();

	// check if this nodes is needed to scale hole or not
//	double a = maxEdge(newHole);
//	double b = pow(alpha_, (double)1 / (double)(level_ + 1)) - 1;
//	double c = 1 / cos(2 * M_PI / numNode(newHole)) - 1;
//	double g = a / b * c;

///	double g = maxEdge(newHole) / (pow(alpha_, 1 / (level_ + 1)) - 1) * ( 1 / cos(2 * M_PI / numNode(newHole)) - 1);

	if (numNode(newHole) > 3 && distance(newHole, this) > sgh->g)
	{
		this->level_++;
		reduce(newHole);

		// add to hole_list_
		hole_list_ = newHole;

		drop(p, "Reduce");
		sendBCH(newHole);
		dumpApproximateHole();
	//	printf("%d (%f - %f)\t- Drop BCH %d numNode %d\n", my_id_, x_, y_, cmh->uid_, numNode(newHole));
	}
	else
	{
		// add to hole_list_
		hole_list_ = newHole;

		// broadcast message
		cmh->direction_ = hdr_cmn::DOWN;
		cmh->last_hop_  = my_id_;

		// printf("%d (%f - %f) - forward BCH %d\n", my_id_, x_, y_, cmh->uid_);
		send(p, 0);
	}

//	// dump
//	dumpBroadcast();
}

// ------------------------ Reduce hole ------------------------ //
//
//void ScaleGoalAgent::reduce(polygonHole* h)
//{
//	double min = DBL_MAX;
//	node* re = NULL;
//
//	node* i = h->node_list_;
//	do {
//		Point insp;
//		if (G::intersection(G::line(i, i->next_), G::line(i->next_->next_, i->next_->next_->next_), insp))
//		{
//			double dis = G::distance(i->next_, insp) + G::distance(i->next_->next_, insp) - G::distance(i->next_, i->next_->next_);
//			if (dis < min)
//			{
//				min = dis;
//				re = i;
//			}
//		}
//		i = i->next_;
//	} while (i != h->node_list_);
//
//	// repeat 2 old nodes by insp node
//	node* newNode = new node();
//	G::intersection(G::line(re, re->next_), G::line(re->next_->next_, re->next_->next_->next_), newNode);
//	newNode->next_ = re->next_->next_->next_;
//
//	delete re->next_;			// remove 2 old nodes
//	delete re->next_->next_;
//
//	re->next_ = newNode;
//	h->node_list_ = re;
//}

double
ScaleGoalAgent::distance(polygonHole* h, Point* p)
{
	if (distance_ != -1) return distance_;

	distance_ = DBL_MAX;
	node * n;
	node * i = h->node_list_;
	do {
		double dis = G::distance(this, i->next_);
		if (dis < distance_)
		{
			distance_ = dis;
			n = i;
		}
		i = i->next_;
	} while (i != h->node_list_);

	// n - the nearest node
	// in case destination is distance to vertice:
	return distance_;


	distance_ = DBL_MAX;
	Line bis = G::angle_bisector(n->next_, n, n->next_->next_);
	double dis;

	if (G::position(p, bis) == G::position(n, bis))
			dis = G::distance(p, G::line(n, n->next_));
	else	dis = G::distance(p, G::line(n->next_, n->next_->next_));

	if (dis < distance_) distance_ = dis;

	return distance_;
}

double
ScaleGoalAgent::maxEdge(polygonHole* h)
{
	double re = DBL_MIN;
	node * i = h->node_list_;
	do
	{
		double dis = G::distance(i, i->next_);
		if (dis > re) re = dis;
		i = i->next_;
	} while (i->next_ && i != h->node_list_);

	return re;
}

int
ScaleGoalAgent::numNode(polygonHole* h)
{
	int num = 0;
	node * i = h->node_list_;
	do {
		num++;
		i = i->next_;
	} while(i && i != h->node_list_);

	return num;
}

// ------------------------ Dump ------------------------ //

void ScaleGoalAgent::dumpBroadcast()
{
	if (level_ % 2)
	{
		FILE *fp = fopen("BroadcastRegion.tr", "a+");
		fprintf(fp, "%d	%f	%f	%d\n", my_id_, x_, y_, level_);
		fclose(fp);
	}
}

// ------------------------ End ------------------------ //
