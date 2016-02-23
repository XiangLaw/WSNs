#include <routing/address.h>
#include "taagent.h"
#include "taagent_packet.h"

int hdr_taagent::offset_;

/*
 * TAAgent Header Class
 */
static class TAAgentHeaderClass : public PacketHeaderClass {
public:
    TAAgentHeaderClass() : PacketHeaderClass("PacketHeader/TAAGENT", sizeof(hdr_taagent)) {
        bind_offset(&hdr_taagent::offset_);
    }

    ~TAAgentHeaderClass() { }
} class_taagenthdr;

/*
 * TAAgent Class
 */
static class TAAgentClass : public TclClass {
public:
    TAAgentClass() : TclClass("Agent/TAAGENT") { }

    TclObject *create(int, const char *const *) {
        return new TAAgent();
    }
} class_taagent;

/*
 * Agent
 */
TAAgent::TAAgent() : Agent(PT_TAAGENT) {
    my_id_ = -1;
    x_ = -1;
    y_ = -1;

    node_ = NULL;
    port_dmux_ = NULL;
    trace_target_ = NULL;
    neighbor_list_ = NULL;

    dest = new Point();
}

int TAAgent::command(int argc, const char *const *argv) {
    if (argc == 2) {
        if (strcasecmp(argv[1], "start") == 0) {
            startUp();
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "dump") == 0) {
            dumpNeighbor();
            dumpEnergy();
            return TCL_OK;
        }
        if (strcasecmp(argv[1], "nodeoff") == 0) {
            if (node_->energy_model()) {
                node_->energy_model()->update_off_time(true);
            }
        }
    }

    if (argc == 3) {
        if (strcasecmp(argv[1], "addr") == 0) {
            my_id_ = Address::instance().str2addr(argv[2]);
            return TCL_OK;
        }

        TclObject *obj;
        if ((obj = TclObject::lookup(argv[2])) == 0) {
            fprintf(stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1], argv[2]);
            return (TCL_ERROR);
        }
        if (strcasecmp(argv[1], "node") == 0) {
            node_ = (MobileNode *) obj;
            return (TCL_OK);
        }
        else if (strcasecmp(argv[1], "port-dmux") == 0) {
            port_dmux_ = (PortClassifier *) obj; //(NsObject *) obj;
            return (TCL_OK);
        }
        else if (strcasecmp(argv[1], "tracetarget") == 0) {
            trace_target_ = (Trace *) obj;
            return TCL_OK;
        }
    }// if argc == 3

    return (Agent::command(argc, argv));
}

void TAAgent::recv(Packet *p, Handler *h) {
    struct hdr_cmn *cmh = HDR_CMN(p);

    switch (cmh->ptype()) {
        case PT_CBR:
        default:
            drop(p, "NoRoute");
            break;
    }
}

void TAAgent::startUp() {
    this->x_ = node_->X();        // get Location
    this->y_ = node_->Y();

    FILE *fp;
    fp = fopen("Neighbors.tr", "w");
    fclose(fp);

    if (node_->energy_model()) {
        fp = fopen("Energy.tr", "w");
        fclose(fp);
    }
}

/*
 * Dump
 */
void TAAgent::dumpEnergy() {
    if (node_->energy_model()) {
        FILE *fp = fopen("Energy.tr", "a+");
        fprintf(fp, "%d\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", my_id_, this->x_, this->y_,
                node_->energy_model()->energy(),
                node_->energy_model()->off_time(),
                node_->energy_model()->et(),
                node_->energy_model()->er(),
                node_->energy_model()->ei(),
                node_->energy_model()->es()
        );
        fclose(fp);
    }
}

void TAAgent::dumpNeighbor() {
    FILE *fp = fopen("Neighbors.tr", "a+");
    fprintf(fp, "%d	%f	%f	%f	", this->my_id_, this->x_, this->y_, node_->energy_model()->off_time());
    for (node *temp = neighbor_list_; temp; temp = temp->next_) {
        fprintf(fp, "%d,", temp->id_);
    }
    fprintf(fp, "\n");
    fclose(fp);
}
