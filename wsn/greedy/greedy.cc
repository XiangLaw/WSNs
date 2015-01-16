#include <vector>

#include "greedy.h"

using namespace std;

int hdr_greedy::offset_;

/**
 * Greedy Packer header class
 */
static class GreedyHeaderClass : public PacketHeaderClass{
	public:
		GreedyHeaderClass() : PacketHeaderClass("PacketHeader/Greedy", sizeof(hdr_all_greedy))
		{
			bind_offset(&hdr_greedy::offset_);
		}
		~GreedyHeaderClass(){}
} class_greedyhdr;


/**
 * Greedy Agent class
 */
static class GreedyAgentClass : public TclClass {
public:
	GreedyAgentClass() : TclClass("Agent/Greedy") {}
	TclObject *create(int argc, const char*const* argv) 
	{
		return (new GreedyAgent());
	}
} class_GreedyAgent;

/// Agent
GreedyAgent::GreedyAgent() : Agent(PT_GREEDY), addr_(-1) , x_(0.0), y_(0.0) {
}

int GreedyAgent::command(int argc, const char*const* argv) {
	if (argc == 2) {
		if (strcasecmp(argv[1], "start") == 0) {
			startUp(); 		// should i call startup to run simulator?
			return TCL_OK;
		} else if (strcasecmp(argv[1], "set-location") == 0){
			setLocation();	// set destination location
			return TCL_OK;
		}
	/// have no idea what is that stuff is doing so just ignore it	
	} else if (argc == 3) {
		if (strcmp(argv[1], "port-dmux") == 0) {
			port_dmux_ = (PortClassifier*)TclObject::lookup(argv[2]);
			if (port_dmux_ == 0) {
				fprintf(stderr, "%s: %s lookup of %s failed \n", __FILE__, argv[1], argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		} else if (strcmp(argv[1], "node") == 0) {
			node_ = (MobileNode*)TclObject::lookup(argv[2]);
			if (node_ == 0) {
				fprintf(stderr,  "%s: %s lookup of %s failed \n", __FILE__, argv[1], argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		} else if (strcmp(argv[1], "addr") == 0) {
			addr_ = Address::instance().str2addr(argv[2]);
			return TCL_OK;
		} else if (strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
			logtarget_ = (Trace*)TclObject::lookup(argv[2]);
			if (logtarget_ == 0) {
				fprintf(stderr, "%s: %s lookup of %s failed \n", __FILE__, argv[1], argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
	} else if (argc == 5) {
		if (strcmp(argv[1], "set-nbr") == 0) {
			addGreedyNbr(Address::instance().str2addr(argv[2]), atof(argv[3]), atof(argv[4]));
			return TCL_OK;
		}
		/*if (strcmp(argv[1], "set-flow-data") == 0) {
			addFlow(Address::instance().str2addr(argv[2]) , atof(argv[3]), atof(argv[4]));
			return TCL_OK;
		}*/
	} 
	return Agent::command(argc, argv);
}

void GreedyAgent::startUp() {
	// do nothing
}

void GreedyAgent::recv(Packet* p, Handler* h) {
	struct hdr_cmn* ch = HDR_CMN(p);
	struct hdr_ip* ih = HDR_IP(p);

	if(ch->ptype() == PT_GREEDY) { // this is our expecting packet
		routing(p);
	}
	return drop(p, "NotGreedyAgentPacket");

	/*
	if (ih->saddr() == addr_) {
		// if there exists a loop and the packet is not DATA packet, drops the packet
		if (ch->num_forwards() > 0 && ch->ptype() == PT_GREEDY) {
			// loop
			drop(p, DROP_RTR_ROUTE_LOOP);
			return;	
		} else if (ch->num_forwards() == 0) {
			struct hdr_greedy_data *hdr = HDR_GREEDY_DATA(p);
		
			// else if this is a packet this node is originating
			// in reality, geographical routing does not use IP, so we do not add IP header
			ch->size() += hdr->size();
			
			GreedyFlow *td = getFlow(ih->daddr());

			hdr->type_ = GREEDY_PKT_DATA;
			hdr->src_ = addr_;
			hdr->dest_ = ih->daddr();
			hdr->destX_ = td->destX_;
			hdr->destY_ = td->destY_;
			
			ih->ttl_ = 100; // change ttl, the default ttl is 32
		}
	}

	// handling incoming packet
	if (ch->ptype() == PT_GREEDY) {
		struct hdr_greedy* hdr = HDR_GREEDY(p);
		switch (hdr->type_) {
			default:
				printf("Error with Greedy packet type. \n");
				exit(1);
		}
	} else {
		//ih->ttl_--;
		if (ih->ttl_ == 0) {
			drop(p, DROP_RTR_TTL);
			return;
		}
		forwardData(p);
	}
	*/
}

void GreedyAgent::routing(Packet *p) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ip  = HDR_IP(p);

	if(ip->daddr() == addr_) { 				// if this agent is destination
		if(ch->direction() == hdr_cmn::UP) { 	// someone sends something
			port_dmux_->recv(p, 0);
			return;
		} 
		else {									// me sends to me
			drop(p, "MeSendToMySelf?"); 		
			return;
		}
	}

	if(ip->ttl_ == 0) { 						// time out
		drop(p, DROP_RTR_TTL);
		return;
	}

	/**
	 * greedy routing
	 */
	nsaddr_t target = getNextNode(p); 			// choose the best neighbor
	if(target == -1) { 							// locally minumun
		drop(p, "LocallyMinimun");
		return;
	}

	// handling forware the packet
	ch->direction() = hdr_cmn::DOWN;
	ch->addr_type() = NS_AF_INET;
	ch->next_hop_ = target;
	ch->last_hop_ = addr_;
	ch->num_forwards_++;						// what's the meaning of num_forwards_?
	ip->ttl_--;									// decrease time to live

	send(p, 0);
}

void GreedyAgent::forwardData(Packet *p) {
	struct hdr_cmn *ch = HDR_CMN(p);
	struct hdr_ip *ih = HDR_IP(p);

	if (ch->direction() == hdr_cmn::UP &&
		((u_int32_t)ih->daddr() == IP_BROADCAST || ih->daddr() == addr_)) {
		port_dmux_->recv(p, 0);
		return;
	} else {
		/*
		/ geedy fowarding
		*/		
		nsaddr_t target = getNextNode(p);
		if (target == -1) {
			Packet::free(p);
			return;
		}

		// handling forward the packet
		ch->direction() = hdr_cmn::DOWN;
		ch->addr_type() = NS_AF_INET;
		ch->next_hop_ = target;
		ch->last_hop_ = addr_;
		ch->num_forwards_++;
		
		send(p, 0);
	}
}

nsaddr_t GreedyAgent::getNextNode(Packet *p) {
	struct hdr_greedy_data *hdr = HDR_GREEDY_DATA(p);
	
	float minDist = sqrt(pow(hdr->destX_ - x_, 2) + pow(hdr->destY_ - y_, 2));
	nsaddr_t target = -1;
	for (std::vector< GreedyNbr* >::iterator it = nbr_.begin(); it != nbr_.end(); it++) {
		if (hdr->dest_ == (*it)->addr_) return (*it)->addr_;
		float d = sqrt(pow(hdr->destX_ - (*it)->x_, 2) + pow(hdr->destY_ - (*it)->y_, 2));
		if (d < minDist) target = (*it)->addr_;
	}

	return target;
}

void GreedyAgent::setLocation() {
	x_ = (float)node_->X();
	y_ = (float)node_->Y();
}

bool GreedyAgent::addGreedyNbr(nsaddr_t id, float x, float y) {	
	// search exists nbrs and update
	for (std::vector< GreedyNbr* >::iterator it = nbr_.begin(); it != nbr_.end(); it++) {
		if ((*it)->addr_ == id) {
			(*it)->x_ = x;
			(*it)->y_ = y;
			return false;
		}
	}
	
	// add new nbr
	GreedyNbr *nn = new GreedyNbr(id, x, y);
	nbr_.push_back(nn);
	return true;
}

void GreedyAgent::dumpPacket(Packet *p) {
	struct hdr_greedy_data *hdr = HDR_GREEDY_DATA(p);
	fprintf(stderr, "pkt -n %d -n %d -s %d -d %d -h %d",
			 addr_, HDR_CMN(p)->next_hop(), hdr->src_, hdr->dest_, (HDR_CMN(p))->num_forwards());
}
