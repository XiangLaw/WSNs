#include "gear.h"
#include "wsn/geomathhelper/geo_math_helper.h"

int hdr_gear_offset::offset_;

#define MAX(a, b) ((a) > (b) ? (a) : (b))

static class GEARHeaderClass : public PacketHeaderClass {
public:
	GEARHeaderClass() : PacketHeaderClass("PacketHeader/GEAR", sizeof(hdr_all_gear)) {
		bind_offset(&hdr_gear_offset::offset_);
	}
}class_gearhdr;

static class GEARAgentClass : public TclClass {
public:
	GEARAgentClass() : TclClass("Agent/GEAR") {}
	TclObject* create(int, const char*const*) {
		return (new GEARAgent());
	}
}class_gear;

void GEARTimer::expire(Event *e) {
	(a_->*firing_)();
}

void GEARAgent::helloTimeOut() {
	sendHello();
	hello_timer_.resched(hello_period_);
}

// update energy-aware value regularly
void GEARAgent::updateEnergyTimeOut() {
	h_ = alpha_ * G::distance(my_x_, my_y_, des_x_, des_y_) + (1 - alpha_) *
			(node_->energy_model()->initialenergy() - node_->energy_model()->energy());

	// update energy statistic for each 30 seconds
	update_energy_timer_.resched(60);
}

GEARAgent::GEARAgent() : Agent(PT_GEAR),
		hello_timer_(this, &GEARAgent::helloTimeOut),
		update_energy_timer_(this, &GEARAgent::updateEnergyTimeOut){
	my_id_ = -1;
	my_x_ = -1;
	my_y_ = -1;

	des_x_ = -1;
	des_y_ = -1;

	alpha_ = 0.8;

	bind("hello_period_", &hello_period_);
	bind("range_", &range_);
	bind("alpha_", &alpha_);

	nblist_ = new GNeighbors();
}

// using at start time. get its address and location
inline void GEARAgent::getMyLocation() {
	my_x_ = node_->X();
	my_y_ = node_->Y();
}

inline void GEARAgent::getDestInfo() {
	des_x_ = node_->destX();
	des_y_ = node_->destY();
}

void GEARAgent::sendHello() {
	if (my_id_ < 0) return;

	Packet *p = allocpkt();
	struct hdr_cmn *cmh = HDR_CMN(p);
	struct hdr_ip *iph = HDR_IP(p);
	struct hdr_gear_hello *hh = HDR_GEAR_HELLO(p);

	// insert information to common header in packet
	cmh->next_hop_ = IP_BROADCAST;
	cmh->last_hop_ = my_id_;
	cmh->addr_type_ = NS_AF_INET;
	cmh->ptype() = PT_GEAR;
	cmh->size_ = IP_HDR_LEN + hh->size();

	// insert ip header
	iph->daddr() = IP_BROADCAST;
	iph->saddr() = my_id_;
	iph->sport() = RT_PORT;
	iph->ttl_ = 32;

	// insert location
	hh->type_ = HELLO;
	hh->x_ = my_x_;
	hh->y_ = my_y_;
	//hh->e_ = node_->energy_model()->initialenergy() - node_->energy_model()->energy();
	hh->e_ = node_->energy_model()->energy();
	send(p, 0);
}

void GEARAgent::recvHello(Packet *p) {
	struct hdr_cmn *cmh = HDR_CMN(p);
	struct hdr_gear_hello *hh = HDR_GEAR_HELLO(p);

	// insert new neighbor to neighbor list
	nblist_->newnb(cmh->last_hop_, hh->x_, hh->y_, hh->e_);

	drop(p, "HELLO");
}

void GEARAgent::recvGEAR(Packet *p) {
	hdr_cmn *cmh = HDR_CMN(p);
	hdr_ip *iph = HDR_IP(p);
	hdr_gear *gh = HDR_GEAR(p);

	if (iph->ttl_-- < 0) {
		drop(p, "TTL");
		return;
	}

	gneighbor *next = getNeighborByGreedy(gh->des_x_, gh->des_y_);

	//if (next != NULL) {
	//	cmh->next_hop_ = next->id_;
	//}
	//else {
		next = getNeighborByEnergyAware(cmh->last_hop_);

		if (next == NULL) {
			printf("\nError");
			drop(p, "ERROR");
			return;
		}

		cmh->next_hop_ = next->id_;

		//printf("%d ", cmh->next_hop_);

		updateEnergyAware(next);

		if (my_id_ != iph->saddr())
			sendBack(cmh->last_hop_);
	//}

	cmh->last_hop_ = my_id_;
	cmh->direction_ = hdr_cmn::DOWN;
	cmh->addr_type() = NS_AF_INET;

	send(p, 0);
}

void GEARAgent::sendBack(nsaddr_t id) {
	if (my_id_ < 0) return;

	Packet *p = allocpkt();
	struct hdr_cmn *cmh = HDR_CMN(p);
	struct hdr_ip *iph = HDR_IP(p);
	struct hdr_gear_send_back *sbh = HDR_GEAR_SEND_BACK(p);

	// insert information to common header in packet
	cmh->next_hop_ = id;
	cmh->last_hop_ = my_id_;
	cmh->addr_type_ = NS_AF_INET;
	cmh->ptype() = PT_GEAR;
	cmh->size_ = IP_HDR_LEN + sbh->size();

	// insert ip header
	//iph->daddr() = id;
	iph->saddr() = my_id_;
	iph->sport() = RT_PORT;

	// insert location
	sbh->type_ = SENDBACK;
	sbh->h_ = h_;
	send(p, 0);
}

void GEARAgent::recvSendBack(Packet *p) {
	hdr_cmn *cmh = HDR_CMN(p);
	hdr_gear_send_back *sbh = HDR_GEAR_SEND_BACK(p);

	for (int i = 0; i < nblist_->nbSize(); i++) {
		if (nblist_->getnb_at(i)->id_ == cmh->last_hop_) {
			nblist_->getnb_at(i)->h_ = sbh->h_;
			break;
		}
	}

	drop(p, "SB");
}

void GEARAgent::updateEnergyAware(gneighbor* next) {
	int constEnergy = 2;

	h_ = next->h_ + constEnergy;
}

struct gneighbor* GEARAgent::getNeighborByEnergyAware(nsaddr_t id) {
	double min = 999999;
	struct gneighbor* temp = nblist_->getnb_at(0);

	if (temp == NULL) return NULL;

	double h, e;
	for (int i = 0; i < nblist_->nbSize(); i++) {
		struct gneighbor* curr = nblist_->getnb_at(i);

		if (curr->id_ == id)
			continue;

		h = curr->h_;

		if (h == -1) {
			e = curr->e_;
			h = alpha_ * G::distance(curr->x_, curr->y_, des_x_, des_y_) + (1 - alpha_)*e;
		}

		if (h <= min && curr->id_ != id) {
			temp = curr;
			min = h;
		}
	}

	temp->h_ = min;

	return temp;
}

struct gneighbor* GEARAgent::getNeighborByGreedy(double x, double y) {
	struct gneighbor* temp = nblist_->getnb_at(0);
	double min = G::distance(temp->x_, temp->y_, x, y);

	for (int i = 1; i < nblist_->nbSize(); i++) {
		struct gneighbor* nb = nblist_->getnb_at(i);

		if (G::distance(nb->x_, nb->y_, x, y) < min) {
			min = G::distance(nb->x_, nb->y_, x, y);
			temp = nb;
		}
	}

	if (min < G::distance(my_x_, my_y_, x, y))
		return temp;
	else return NULL;
}

void GEARAgent::recv(Packet *p, Handler *h) {
	struct hdr_cmn *cmh = HDR_CMN(p);
	struct hdr_ip *iph = HDR_IP(p);
	struct hdr_gear* gh = HDR_GEAR(p);

	// if packet is from higher layer, prepare header for angleview before forward packet
	// convert PT_CBR to PT_GEAR
	if (my_id_ == iph->saddr()) {
		if (cmh->num_forwards() == 0 && cmh->ptype() != PT_GEAR) {
			cmh->ptype() = PT_GEAR;
			cmh->size() += IP_HDR_LEN + gh->size();

			// set up information for gear header
			gh->type_ = GEAR;
			gh->des_x_ = des_x_;
			gh->des_y_ = des_y_;

			iph->ttl_ = 100;
		}
	}

	if (cmh->ptype() == PT_GEAR) {
		struct hdr_gear_offset *goh = HDR_GEAR_OFFSET(p);
		switch (goh->type_) {
		case HELLO:
			recvHello(p);
			break;
		case GEAR:
			if (gh->des_x_ == my_x_ && gh->des_y_ == my_y_) {
				printf("recved.\n");
				port_dmux_->recv(p, h);
			}
			recvGEAR(p);
			break;
		case SENDBACK:
			recvSendBack(p);
			break;
		default:
			break;
		}
	}
}

int GEARAgent::command(int argc, const char*const* argv) {
	if (argc == 2) {
		if (strcmp(argv[1], "start") == 0) {
			startUp();
			return TCL_OK;
		}
		if (strcasecmp(argv[1], "dump") == 0)
		{
			dumpNeighbors();
			dumpEnergy();
			return TCL_OK;
		}
	}

	if(argc==3){
	    if(strcasecmp(argv[1], "addr")==0){
	      my_id_ = Address::instance().str2addr(argv[2]);
	      return TCL_OK;
	    }
	    TclObject *obj;
	     if ((obj = TclObject::lookup (argv[2])) == 0){
	       fprintf (stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1], argv[2]);
	       return (TCL_ERROR);
	     }
	     if (strcasecmp (argv[1], "node") == 0) {
	       node_ = (MobileNode*) obj;
	       return (TCL_OK);
	     }
	     else if (strcasecmp (argv[1], "port-dmux") == 0) {
	       port_dmux_ = (PortClassifier*) obj; //(NsObject *) obj;
	       return (TCL_OK);
	     } else if(strcasecmp(argv[1], "tracetarget")==0){
	       tracetarget = (Trace *)obj;
	       return TCL_OK;
	     }
	}
	return Agent::command(argc,argv);
}

void GEARAgent::startUp() {
	getMyLocation();
	getDestInfo();

	FILE *fp;
	fp = fopen("Neighbors.tr",	"w");	fclose(fp);

	if (node_->energy_model())
	{
		fp = fopen("Energy.tr", 	"w");	fclose(fp);
	}

	nblist_->myinfo(my_id_, my_x_, my_y_);
	hello_timer_.resched(randSend_.uniform(0.5, 5));
	update_energy_timer_.resched(randSend_.uniform(0, 0.5));
}

void GEARAgent::dumpEnergy() {
	FILE *fp = fopen("Energy.tr", "a+");
	fprintf(fp, "%d\t%f\t%f\t%f\n", my_id_, my_x_, my_y_, node_->energy_model()->energy());
	fclose(fp);
}

void GEARAgent::dumpNeighbors(){
	FILE *fp = fopen("Neighbors.tr", "a+");

	fprintf(fp, "%d	%f	%f	%f	", my_id_, my_x_, my_y_, -1.0);
	nblist_->dump(fp);

	fclose(fp);
}
