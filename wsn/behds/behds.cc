//
// Created by Vu Quoc Huy  on 8/27/15.
//

#include <packet.h>
#include <wsn/behds/behds_packet.h>
#include <wsn/boundhole/boundhole.h>
#include "behds.h"

int hdr_behds::offset_;

static class BEHDSHeaderClass : public PacketHeaderClass {
    public:
    BEHDSHeaderClass() : PacketHeaderClass("PacketHeader/BEHDS", sizeof(hdr_all_behds))
    {
        bind_offset(&hdr_behds::offset_);
    }
} class_behdshdr;

static class BEHDSAgentClass : public TclClass {
    public:
    BEHDSAgentClass() : TclClass("Agent/BEHDS") {}
    TclObject* create(int, const char*const*) {
        return (new BEHDSAgent());
    }
}class_behds;

/********** Agent *************/
BEHDSAgent::BEHDSAgent() : BoundHoleAgent()
{
    circleHole_list_ = NULL;
    routing_num_ = 0;
}

void
BEHDSAgent::recv(Packet* p, Handler* h)
{
    struct hdr_cmn* cmh = HDR_CMN(p);

    if (cmh->ptype() == PT_BEHDS)
        recvBEHDS(p);
    else
        BoundHoleAgent::recv(p, h);
}

int
BEHDSAgent::command(int argc, const char*const* argv)
{
    return BoundHoleAgent::command(argc, argv);
}

// --------------------- Approximate hole ------------------------- //
void
BEHDSAgent::createHole(Packet* p)
{
    polygonHole* h = createPolygonHole(p);

    // create new circleHole
    circleHole* newHole = new circleHole();
    newHole->hole_id_	= h->hole_id_;
    newHole->next_		= circleHole_list_;
    circleHole_list_	= newHole;

    // approximate hole to circleHole
    double minx = 9999;
    double miny = 9999;
    double maxx = 0;
    double maxy = 0;
    for (struct node* ntemp = h->node_list_; ntemp; ntemp = ntemp->next_)
    {
        minx = minx < ntemp->x_ ? minx : ntemp->x_;
        miny = miny < ntemp->y_ ? miny : ntemp->y_;
        maxx = maxx > ntemp->x_ ? maxx : ntemp->x_;
        maxy = maxy > ntemp->y_ ? maxy : ntemp->y_;
    }
    newHole->x_ = (minx + maxx) / 2;
    newHole->y_ = (miny + maxy) / 2;

    // calculate radius
    newHole->radius_ = 0;
    for (node* ntemp = h->node_list_; ntemp; ntemp = ntemp->next_)
    {
        double d = G::distance(newHole, ntemp);
        newHole->radius_ = newHole->radius_ > d ? newHole->radius_ : d;
    }

    delete h;
}

void
BEHDSAgent::sendHA(Packet* p, nsaddr_t saddr, Point spos)
{
    struct hdr_cmn*			cmh = HDR_CMN(p);
    struct hdr_ip*			iph = HDR_IP(p);
    struct hdr_behds_ha*	ehh = HDR_BEHDS_HA(p);

    cmh->size()	 = IP_HDR_LEN + ehh->size();

    iph->daddr() = saddr;
    iph->saddr() = my_id_;
    iph->sport() = RT_PORT;
    iph->dport() = RT_PORT;
    iph->ttl_	 = IP_DEF_TTL;

    ehh->type_              = BEHDS_DATA_HA;
    ehh->id_	 			= circleHole_list_->hole_id_;
    ehh->circle_.x_ 		= circleHole_list_->x_;
    ehh->circle_.y_ 		= circleHole_list_->y_;
    ehh->circle_.radius_ 	= circleHole_list_->radius_;
    ehh->spos_				= spos;

    recvBEHDS(p);
}

void
BEHDSAgent::recvBEHDS(Packet* p)
{
    hdr_cmn*		cmh = HDR_CMN(p);
    hdr_ip*			iph = HDR_IP(p);
    hdr_behds_ha*	edh = HDR_BEHDS_HA(p);

    // forward packet
    if (cmh->direction() == hdr_cmn::UP	&& iph->daddr() == my_id_)	// up to destination
    {
        // add new circle hole
        circleHole* newHole = (circleHole*)malloc(sizeof(circleHole));
        newHole->hole_id_	= edh->id_;
        newHole->radius_ 	= edh->circle_.radius_;
        newHole->x_			= edh->circle_.x_;
        newHole->y_			= edh->circle_.y_;
        newHole->next_		= circleHole_list_;
        circleHole_list_	= newHole;

        routing();

        edh->type_ = BEHDS_DATA_ROUTING;
        recvData(p);
    }
    else
    {
        // -------- forward by greedy
        node * nexthop = getNeighborByGreedy(edh->spos_);

        if (nexthop == NULL)	// no neighbor close
        {
            drop(p, DROP_RTR_NO_ROUTE);
            return;
        }
        else
        {
            cmh->direction() = hdr_cmn::DOWN;
            cmh->addr_type() = NS_AF_INET;
            cmh->last_hop_ = my_id_;
            cmh->next_hop_ = nexthop->id_;
            send(p, 0);
        }
    }
}

// --------------------- Routing ---------------------------------- //

void
BEHDSAgent::routing()
{
    // clear routing table
    routing_num_ = 0;

    if (dest->x_ || dest->y_)
    {
        // Add destination to routing table
        addrouting(dest);

        // create routing table
        Line SD = G::line(this, dest);

        for (circleHole* h = circleHole_list_; h; h = h->next_)
        {
            // check if need to routing to avoid this hole
            if (G::distance(h, SD) < h->radius_ && G::distance(this, h) >= h->radius_ && G::distance(dest, h) >= h->radius_)
            {
                // calculate temp destination
                Point s1, s2, d1, d2;

                G::tangent_point(h, this, s1, s2);
                G::tangent_point(h, dest, d1, d2);

                Line ts = G::is_intersect(this, dest, h, &s1) ? G::line(this, s1) : G::line(this, s2);
                Line td = G::is_intersect(this, dest, h, &d1) ? G::line(dest,  d1) : G::line(dest,  d2);

                Point in;
                if (G::intersection(ts, td, in))
                    addrouting(in);

            } // if (d < r)
        } // for each hole
    } // if (dest->x_ || dest->y_)
    // Add source to routing table
    addrouting(this);
    routing_num_--;			// go to myself, remove "this" from routing table
}

void
BEHDSAgent::addrouting(Point* p)
{
    for (int i = 0; i < routing_num_; i++)
    {
        if (routing_table[i].x_ == p->x_ && routing_table[i].y_ == p->y_)
            return;
    }

    routing_table[routing_num_].x_ = p->x_;
    routing_table[routing_num_].y_ = p->y_;
    routing_num_++;
}


// --------------------- Send data -------------------------------- //
/*
 * send new CBR packet
 */
void
BEHDSAgent::sendData(Packet* p)
{
    hdr_cmn*			cmh = HDR_CMN(p);
    hdr_ip*				iph = HDR_IP(p);
    hdr_behds_data* 	edh = HDR_BEHDS_DATA(p);

    cmh->size() += IP_HDR_LEN + edh->size();
    cmh->direction_ = hdr_cmn::DOWN;

    edh->type_ 			= BEHDS_DATA_GREEDY; // always start with greedy to destination
    edh->daddr_ 		= iph->daddr();
    edh->vertex_num_	= (u_int8_t) routing_num_;
    for (int i = 0; i <= routing_num_; i++)
    {
        edh->vertex[i] = routing_table[i];
    }

    iph->saddr() = my_id_;
    iph->daddr() = -1;
    iph->ttl_ 	 = 3 * IP_DEF_TTL;
}

void
BEHDSAgent::recvData(Packet* p)
{
    struct hdr_cmn*				cmh = HDR_CMN(p);
    struct hdr_ip*				iph = HDR_IP(p);
    struct hdr_behds_data* 	edh = HDR_BEHDS_DATA(p);

    if (cmh->direction() == hdr_cmn::UP	&& edh->daddr_ == my_id_)	// up to destination
    {
        dumpHopcount(p);
        port_dmux_->recv(p, 0);
        return;
    }
    else
    {
        // meet hole
        if (circleHole_list_ && iph->saddr() != my_id_ && edh->type_ == BEHDS_DATA_GREEDY)
        {
            sendHA(p, iph->saddr(), edh->vertex[edh->vertex_num_]);
        }
        else if (edh->type_ == BEHDS_DATA_HA) {
            recvBEHDS(p);
        }
        else {
            // -------- forward by greedy
            node * nexthop = NULL;
            while (nexthop == NULL && edh->vertex_num_ > 0)
            {
                nexthop = getNeighborByGreedy(edh->vertex[edh->vertex_num_ - 1]);
                if (nexthop == NULL)
                    edh->vertex_num_--;
            }

            if (nexthop == NULL)	// no neighbor close
            {
                drop(p, DROP_RTR_NO_ROUTE);
                return;
            }
            else
            {
                cmh->direction() = hdr_cmn::DOWN;
                cmh->addr_type() = NS_AF_INET;
                cmh->last_hop_ = my_id_;
                cmh->next_hop_ = nexthop->id_;
                send(p, 0);
            }
        }
    }
}
