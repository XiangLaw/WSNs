/*
 * dynamicpolgyon.cc
 *
 *  Last edited on Nov 14, 2013
 *  by Trong Nguyen
 */
 
#include <cstdint>
#include "config.h"
#include "dynamicpolygon.h"
#include "wsn/geomathhelper/geo_math_helper.h"

#include "../include/tcl.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int hdr_dynamicpolygon::offset_;

static class DynamicPolygonHeaderClass : public PacketHeaderClass {
public:
    DynamicPolygonHeaderClass() : PacketHeaderClass("PacketHeader/DYNAMICPOLYGON", sizeof(hdr_dynamicpolygon)) {
        bind_offset(&hdr_dynamicpolygon::offset_);
    }
} class_dynamicpolygonhdr;

static class DynamicPolygonAgentClass : public TclClass
{
	public:
		DynamicPolygonAgentClass() : TclClass("Agent/DYNAMICPOLYGON") {}
		TclObject* create(int, const char*const*)
		{
			return (new DynamicPolygonAgent());
		}
}class_dynamic;

// ------------------------ Agent ------------------------ //

DynamicPolygonAgent::DynamicPolygonAgent() : ConvexHullAgent(),
                                             broadcastTimer(this)
{
    f_ = 0;

	bind("alpha_", &alpha_);
}

int
DynamicPolygonAgent::command(int argc, const char*const* argv)
{
	if (argc == 2)
	{
		if (strcasecmp(argv[1], "start") == 0)
		{
			startUp();
		}
        if (strcasecmp(argv[1], "dump") == 0){
            dumpRingG();
        }
	}

	return ConvexHullAgent::command(argc,argv);
}

// handle the receive packet just of type PT_DYNAMICPOLYGON
void
DynamicPolygonAgent::recv(Packet *p, Handler *h)
{
	hdr_cmn *cmh = HDR_CMN(p);

	switch (cmh->ptype())
	{
		case PT_HELLO:
			GPSRAgent::recv(p, h);
			break;

		case PT_BOUNDHOLE:
            ConvexHullAgent::recv(p,h);
			break;

		case PT_DYNAMICPOLYGON:
			recvHBI(p);
			break;

		case PT_CBR:
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
			break;

		default:
			drop(p, " UnknowType");
			break;
	}
}

void
DynamicPolygonAgent::startUp()
{
	// clear trace file
	FILE *fp;
	fp = fopen("DynamicPolygon.tr", "w");	fclose(fp);
    fp = fopen("RingG.tr", "w");	fclose(fp);
}

void DynamicPolygonAgent::sendBCH(polygonHole *h) {
    broadcastHBI(h);
}

void DynamicPolygonAgent::broadcastHBI(polygonHole *h) {
    Packet			*p;
    hdr_cmn			*cmh;
    hdr_ip			*iph;
    hdr_dynamicpolygon	*dh;

    p = allocpkt();

    DynamicPolygonPacketData *pkt_data = new DynamicPolygonPacketData();
    hole_list_->unCircleNodeList();
    addData(pkt_data);
    p->setdata(pkt_data);

    cmh = HDR_CMN(p);
    iph = HDR_IP(p);
    dh = HDR_DYNAMICPOLYGON(p);

    cmh->ptype() = PT_DYNAMICPOLYGON;
    cmh->direction() = hdr_cmn::DOWN;
    cmh->next_hop_ = IP_BROADCAST;
    cmh->last_hop_ = my_id_;
    cmh->addr_type_ = NS_AF_INET;
    cmh->size() += IP_HDR_LEN + dh->size();

    iph->daddr() = IP_BROADCAST;
    iph->saddr() = my_id_;
    iph->sport() = RT_PORT;
    iph->dport() = RT_PORT;
    iph->ttl_ = 4 * IP_DEF_TTL;

    dh->n_ = vertex(hole_list_->node_list_);
    dh->g0_ = 0;
    dh->g1_ = calc_g(dh->n_, 1, longestEdge(hole_list_->node_list_)); // formula ()

    send(p, 0);
}

void DynamicPolygonAgent::recvHBI(Packet *pPacket) {
    if (f_) {
        drop(pPacket, "KNOWN_HBI");
    } else {
        hdr_dynamicpolygon* dh = HDR_DYNAMICPOLYGON(pPacket);
        DynamicPolygonPacketData* data = (DynamicPolygonPacketData *) pPacket->userdata();

        polygonHole* hole = createHoleFromPacketData(data);
        double l = distanceToPolygon(hole->node_list_);
        int u = vertex(hole->node_list_);
        int v = hole_list_ == NULL ? INT32_MAX :vertex(hole_list_->node_list_);

        if (l <= dh->g1_ && l >= dh->g0_){
            f_ = 1;
            hole_list_ = hole;

            // broadcast
            broadcastTimer.setParameter(pPacket);
            broadcastTimer.resched(randSend_.uniform(0.0, 0.5));
        } else if (l > dh->g1_){
            polygonHole* p_ = hole;
            double t1 = dh->g0_;
            double t2 = dh->g1_;
            while(true){
                if (!simpifyPolygon(p_)) break;
                double d = longestEdge(p_->node_list_);
                int m = vertex(p_->node_list_);
                l = distanceToPolygon(p_->node_list_);
                t1 = t2;
                t2 = calc_g(dh->n_, dh->n_-m+1, d);
                if (t2 > l) break;
            }
            hole_list_ = p_;
            f_ = 1;
            dh->g0_ = t1;
            dh->g1_ = t2;

            data->removeAllData();
            addData(data);

            // broadcast
            broadcastTimer.setParameter(pPacket);
            broadcastTimer.resched(randSend_.uniform(0.0, 0.5));
        } else if (l < dh->g0_ && u > v) {
            hole_list_ = hole;

//            printf("%d dropped HBI (1)\n", my_id_);
            drop(pPacket, "REPLACE_AND_DROP_HBI_AND");
        } else {
//            printf("%d dropped HBI (2)\n", my_id_);
            drop(pPacket, "DROP_HBI");
        }

    }
}

void DynamicPolygonAgent::addData(DynamicPolygonPacketData *pData) {
    if (hole_list_ == NULL) return;

    // only get first hole
    node* hole = hole_list_->node_list_;
    for (node* tmp = hole; tmp; tmp = tmp->next_){
        pData->addData(*tmp);
    }
}
// ------------------------ Tranfer Data ----------------------- //

void DynamicPolygonAgent::sendData(Packet* p)
{

}

void DynamicPolygonAgent::recvData(Packet* p)
{

}

// ------------------------ Dump ------------------------ //

void DynamicPolygonAgent::dumpBroadcast(){
}

void
DynamicPolygonAgent::dumpBoundhole()
{
	FILE *fp = fopen("DynamicPolygon.tr", "a+");
    fprintf(fp, "id:%d\n", my_id_);

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

void DynamicPolygonAgent::dumpRingG() {
    FILE *fp = fopen("RingG.tr", "a+");
    fprintf(fp, "%d\t%d\t%f\t%f\n", my_id_, hole_list_ == NULL ? 0 : vertex(hole_list_->node_list_), x_, y_);
    fclose(fp);
}


// ------------------------ Helper function ------------------------ //

double DynamicPolygonAgent::distanceToPolygon(node *polygon) {
    node* tmp0;
    node* tmp1;
    node* tmp2;
    node* tmp3;
    Line l, l1, l2; // l: line P(i+1)P(i+2),l1: parallel with P(i)P(i+1), l2: parallel with P(i+2)P(i+3)

    // check if point inside polygon return 0
    if (G::isPointInsidePolygon(this, polygon)) return 0;

    double distance;
    double d = (double)INT32_MAX;
    int n = vertex(polygon);

    for (tmp0 = polygon; n; tmp0 = tmp0->next_, n--){
        tmp1 = tmp0->next_ == NULL ? polygon : tmp0->next_;
        tmp2 = tmp1->next_ == NULL ? polygon : tmp1->next_;
        l = G::line(tmp1, tmp2);

        if (G::position(this, tmp0, &l) > 0) continue;

        distance = G::distance(this, l);
        if (distance < d){
            // detect if node stays on covering polygon's boundary
            tmp3 = tmp2->next_ == NULL ? polygon : tmp2->next_;
            l1 = G::line(tmp0, tmp1);
            l2 = G::line(tmp2, tmp3);

            int pos1 = G::position(this, tmp2, &l1);
            int pos2 = G::position(this, tmp1, &l2);

            if (pos1*pos2 >= 0 && pos1 >= 0){
                d = distance;
            } else if (pos1*pos2 >= 0 && pos1 < 0) {
                if (G::distance(this, l1) <= distance && G::distance(this, l2) <= distance)
                    d = distance;
            } else {
                if ((pos1 < 0 && G::distance(this, l1) <= distance)
                    ||(pos2 < 0 && G::distance(this, l2) <= distance)) {
                    d = distance;
                }
            }
        }
    }

    return d;
}

double DynamicPolygonAgent::longestEdge(node *hole) {
    node* tmp;
    double l;
    double re = 0;
    for (tmp = hole; tmp->next_; tmp = tmp->next_){
        l = G::distance(tmp, tmp->next_);
        re = l > re ? l : re;
    }
    l = G::distance(hole, tmp);
    re = l > re ? l : re;

    return re;
}

polygonHole * DynamicPolygonAgent::createHoleFromPacketData(DynamicPolygonPacketData *data) {
    polygonHole *hole_item = new polygonHole();
    hole_item->node_list_ = NULL;

    node *head = NULL;
    for (int i = 0; i < data->size(); i++) {
        Point n = data->getData(i);
        node *item = new node();
        item->x_ = n.x_;
        item->y_ = n.y_;
        item->next_ = head;
        head = item;
    }

    hole_item->node_list_ = head;
    return hole_item;
}

//simplify polygon, return false if cant simplify anymore
bool DynamicPolygonAgent::simpifyPolygon(polygonHole* polygon) {
    double min = (double)INT32_MAX;
    node* tmp;
    node* a;
    node* b;
    node* c;
    node* d;
    node* q = NULL;
    Point temp_q;
    int n = vertex(polygon->node_list_);

    if (n <= 3) return false;

    for (tmp = polygon->node_list_; n; tmp = tmp->next_, n--){
        node* p0 = tmp == NULL ? polygon->node_list_ : tmp;
        node* p1 = p0->next_ == NULL ? polygon->node_list_ : p0->next_;
        node* p2 = p1->next_ == NULL ? polygon->node_list_ : p1->next_;
        node* p3 = p2->next_ == NULL ? polygon->node_list_ : p2->next_;

        if (!G::intersection(G::line(p0, p1), G::line(p2, p3), temp_q)) continue;
        double distance = G::distance(p1, temp_q) + G::distance(p2, temp_q) - G::distance(p1, p2);
        if (distance < min){
            a = p1;
            b = p2;
            c = p0;
            d = p3;
            if (q == NULL) q = new node();
            q->x_ = temp_q.x_;
            q->y_ = temp_q.y_;
            min = distance;
        }
    }

    if (q == NULL) {
        printf("Couldnt simplified anymore\n");
        return false;
    }
    q->next_ = NULL;
    if (a->next_ == NULL){
        polygon->node_list_ = b->next_;
        c->next_ = q;
    } else if (a->next_->next_ == NULL){
        c->next_ = q;
    } else {
        q->next_ = d;
        if (c->next_ == NULL) {
            polygon->node_list_ = q;
        } else {
            c->next_ = q;
        }
    }

    return true;
}
