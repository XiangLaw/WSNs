#ifndef CORBAL_H_
#define CORBAL_H_

#include <timer-handler.h>
#include <wsn/gpsr/gpsr.h>
#include "../common/struct.h"
#include <iostream>
#include <iterator>
#include <vector>
#include <list>

#include <limits>
#include <queue>

using namespace std;

const int MAX_VERTEX = 100;
#define INT_MAX 99999;

struct edge {
    int id;
    Point a;
    Point b;
    int hole_num_1;
    int hole_num_2;
};

struct sub_list: Point{
    int hole_num;
    int tagent_num;
};
class CorbalAgent;

typedef void(CorbalAgent::*firefunction)(void);

class CorbalTimer : public TimerHandler {
public:
    CorbalTimer(CorbalAgent *a, firefunction f) : TimerHandler() {
        a_ = a;
        firing_ = f;
    }

protected:
    virtual void expire(Event *e);

    CorbalAgent *a_;
    firefunction firing_;
};

class CorbalAgent : public GPSRAgent {
private:
    friend class BoundHoleHelloTimer;

    CorbalTimer findStuck_timer_;
    CorbalTimer boundhole_timer_;
    AgentBroadcastTimer broadcast_timer_;

    void startUp();

    void recvCORBAL(Packet *, Handler *);

    void findStuckAngle();

    void sendBoundHole();

    void recvBoundHole(Packet *);

    node *getNeighborByBoundHole(Point *, Point *);

    void sendHBA(Packet *);

    void recvHBA(Packet *);

    void contructCorePolygonSet(Packet *);

    void isNodeStayOnBoundaryOfCorePolygon(Packet *);

    void addCorePolygonNode(Point, corePolygon *);

    polygonHole *createPolygonHole(Packet *);

    void broadcastHCI();

    void recvHCI(Packet *);

    corePolygon *storeCorePolygons(Packet *);

    corePolygon *chooseRandomCorePolygon(corePolygon *);

    bool canBroadcast(corePolygon *);

    void updatePayload(Packet *, corePolygon *); // update payload with new core polygon information

    void sendData(Packet *);

    void recvData(Packet *);

    double calculateScaleFactor(Packet *, corePolygon *);

    void findViewLimitVertex(Point *N, corePolygon *, node **, node **);

    double distanceToPolygon(Point *, corePolygon *);

    double euclidLengthOfBRSP(Point *, Point *, corePolygon *);

    void addrouting(Point *p, Point *routingTable, u_int8_t &routingCount);

    void bypassHole(Point *, Point *, corePolygon *, corePolygon *, Point *, u_int8_t &);

    node *recvGPSR(Packet *p, Point destionation);

    double range_;
    int limit_max_hop_; // limit_boundhole_hop_
    int limit_min_hop_; // min_boundhole_hop_
    int n_;
    int k_n_;
    double theta_n;
    double epsilon_;
    double gama_;
    double network_width_;
    double network_height_;
    double sin_; //
    int count; //danh so dinh do thi
    double graph[MAX_VERTEX][MAX_VERTEX];
    int core_num;
    int V;
    int boundary_vertices_num;
    int boundhole_counter_;
    int broadcast_counter_;
    int fwd_counter_;

    int sub_num[20];
    int sub_count;
    sub_list sub_node[20];
    sub_list sub_node_final[20];
    RNG fwdProb_;

    polygonHole *hole_;
    corePolygon *core_polygon_set;
    stuckangle *stuck_angle_;
    list<corePolygon*> core_list_st_; // DS ho cat st
    list<corePolygon*> core_list_un_st_; //DS ho khong cat st
    list<corePolygon*> core_list_c_; //Ds ho cat it nhat 1 tiep tuyen cua tap A;
    list<edge> b_list_;
    Point boundary_vertices[40];
    double priority_index_list[20];

    void dumpCorePolygon();

    void dump(Angle, int, int, Line);

    void dumpBroadcastRegion();

    void dumpScaleHole(Packet *, corePolygon *);

    void dumpBigForbiddenArea(Packet *);

    //Point routing(Point*);
    void routing(hdr_corbal_data*);
    void findHolelist(Point, Point);
    void findBlist(node*, node*);
    void findBlist2(node *, node* );
    void findClist(node*, node*);

    void buildGraph();
    int checkTangent(edge, edge);
    //   node checkNode(node*, node*, polygonHole*);
    void findTangent(node*, corePolygon*, int );
    void findTangent2(node*, corePolygon*, int );

    bool is_intersect_poly(corePolygon*, Point, Point); //Line
    bool is_intersect_poly2(corePolygon*, Point, Point);
    bool is_intersect_poly3(Point, Point);
    bool is_intersect_poly4(Point, Point);

    double circuit_poly(corePolygon*);
    double distance_ab(Point a, Point b,corePolygon*);
    int findPointmid_ab(sub_list , sub_list , int);
    int findPointmid_ab2(sub_list , sub_list , int, int);
    double areaCorePolygon(corePolygon*);


    bool is_intersect_line(Line, Point , Point);
    void addEdge( int src, int dest, double cost);

    void dijkstra2(int);
    int minDistance(double dist[], bool sptSet[]);
    int printSolution(double [], int n, int []);
    void printPath(int[],int);
    void scale_path(hdr_corbal_data*);
    Point genrI(corePolygon *hole);

    void dumpHop(hdr_corbal_data*, hdr_ip *);
    void dumpDrop(hdr_ip *iph, int type_);

    //new path
    int Dem = 0;
    int *L;
    char *DanhDau;
    int n,D,C;
    double cost_shortest_path_ ;

    int sub_num_array[100][20];
    int num_array_;
    double cost_sub[100];

    sub_list sub_node_array[100][20];
    sub_list sub_node_final_array[100][20];

    void KhoiTao();
    void InDuongDi(int);
    void TimKiem(int);


public:
    CorbalAgent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);



};
bool
CorbalAgent::is_intersect_poly(corePolygon *hole, Point s, Point t) {
    node* temp = hole->node_;
    int num = 0;
    bool Istersection = false;
    Line l = G::line(s,t);
    while (temp != NULL){
        if (G::distance(s.x_,s.y_,temp->x_, temp->y_) > 0 && G::distance(s.x_,s.y_,temp->next_->x_, temp->next_->y_) > 0) {
            Point a, b;
            a.x_ = temp->x_;
            a.y_ = temp->y_;
            b.x_ = temp->next_->x_;
            b.y_ = temp->next_->y_;
            Istersection = is_intersect_line(l, a, b);
        }
        if (Istersection){
            return Istersection;
        }
        if (temp->next_ == hole->node_){
            return Istersection;
        }
        temp = temp->next_;
    }
    return Istersection;
}

bool
CorbalAgent::is_intersect_poly2(corePolygon *hole, Point s, Point t) {
    node* temp = hole->node_;
    int num = 0;
    bool Istersection = false;
    bool Istersection2 = false;
    Line l = G::line(s,t);
    Line n;
    while (temp != NULL){
        if (G::distance(s.x_,s.y_,temp->x_, temp->y_) > 0 && G::distance(s.x_,s.y_,temp->next_->x_, temp->next_->y_) > 0) {
            Point a, b;
            a.x_ = temp->x_;
            a.y_ = temp->y_;
            b.x_ = temp->next_->x_;
            b.y_ = temp->next_->y_;
            Istersection = is_intersect_line(l,a,b);
            n = G::line(a,b);
            Istersection2 = is_intersect_line(n,s,t);
        }
        if (Istersection && Istersection2){
            return Istersection;
        }
        if (temp->next_ == hole->node_){
            return false;
        }
        temp = temp->next_;
    }
    return false;
}

bool
CorbalAgent::is_intersect_poly4(Point s, Point t) {

    bool Istersection = false;
    corePolygon *tmp = core_polygon_set;
    while(tmp != NULL){
        Istersection = is_intersect_poly2(tmp,s,t);
        if(Istersection) return Istersection;
        tmp = tmp->next_;
    }
    return Istersection;
}

bool
CorbalAgent::is_intersect_line(Line l, Point s, Point t) {
    double a = l.a_ * s.x_ + l.b_ * s.y_ + l.c_;
    double b = l.a_ * t.x_ + l.b_ * t.y_ + l.c_;
    if (a * b > 0) {
        return false;
    } else{
        return true;
    }
}

Point CorbalAgent::genrI(corePolygon *hole) {
    node *n = NULL;
    node *tmp = NULL;
    Point I;
    time_t t;
    srand(time(NULL));

    // --------- scale hole
    // choose I random
    I.x_ = 0;
    I.y_ = 0;

    double fr = 0;
    int a = 0, i =0;
    a = rand() % n_;
    n = hole->node_;

    while (i != a){
        n = n->next_;
        tmp = n;
        i++;
    }

    i =0;
    a = rand() % n_ + 1;
    do {
        int ra = rand();
        I.x_ += n->x_ * ra;
        I.y_ += n->y_ * ra;
        fr += ra;

        n = n->next_;
        i ++;
        if (i == a) break;
    } while (n != tmp );

    I.x_ = I.x_ / fr;
    I.y_ = I.y_ / fr;


    return I;
}

double
CorbalAgent::circuit_poly(corePolygon *p) {
    node* temp = p->node_;
    double cir = 0;
    while(temp->next_ != p->node_ ){
        cir = cir + G::distance(temp->x_,temp->y_,temp->next_->x_,temp->next_->y_);
        temp = temp->next_;
    }
    cir = cir + G::distance(temp->x_,temp->y_,temp->next_->x_,temp->next_->y_);
    return cir;
}

bool
CorbalAgent::is_intersect_poly3(Point s, Point t) {
    corePolygon* tmp = core_polygon_set;
    while (tmp != NULL){
        node* temp = tmp->node_;
        int num = 0;
        bool Istersection = false;
        while (temp != NULL){
            if (G::distance(s.x_,s.y_,temp->x_, temp->y_) > 0 && G::distance(s.x_,s.y_,temp->next_->x_, temp->next_->y_) > 0) {
                Point a, b;
                a.x_ = temp->x_;
                a.y_ = temp->y_;
                b.x_ = temp->next_->x_;
                b.y_ = temp->next_->y_;
                Istersection = G::is_intersect(a,b,s,t);
            }
            if (Istersection){
                return Istersection;
            }
            if (temp->next_ == tmp->node_){
                return Istersection;
            }
            temp = temp->next_;
        }
        return Istersection;
        tmp = tmp->next_;
    }

}

void
CorbalAgent::findTangent2(node *s, corePolygon *tmp, int hole_nb) {
    // create routing table for packet p
    node* S1;	// min angle view of this node to hole
    node* S2;	// max angle view of this node to hole

    // ------------------- S1 S2 - view angle of this node to hole
    double Smax = 0;

    node* i = tmp->node_;
    do {
        for (node* j = i->next_; j != tmp->node_; j = j->next_)
        {
            // S1 S2
            double angle = G::angle(s, i, j);
            if (angle > Smax)
            {
                Smax = angle;
                S1 = i;
                S2 = j;
            }
        }
        i = i->next_;
    } while(i != tmp->node_);

    bool Istersection1 = false, Istersection2 = false;

    for (list<corePolygon *>::iterator iter = core_list_st_.begin(); iter != core_list_st_.end(); iter++) {
        if ((*iter)->id_ != tmp->id_ ) {
            if ((*iter)->id_ == hole_nb || hole_nb == 0 || hole_nb == core_num + 1) {
                Istersection1 = is_intersect_poly(*iter, *s, *S1);
                Istersection2 = is_intersect_poly(*iter, *s, *S2);
            }
        }
    }

    if (!Istersection1 && (hole_nb < tmp->id_ || hole_nb == core_num +1)){
        for (list<corePolygon *>::iterator iter = core_list_un_st_.begin(); iter != core_list_un_st_.end(); iter++) {
            if (is_intersect_poly2(*iter,*s, *S1)){
                core_list_c_.push_back(*iter);
                core_list_un_st_.remove(*iter);
                break;
            }
        }
        for (list<corePolygon *>::iterator iter = core_list_un_st_.begin(); iter != core_list_un_st_.end(); iter++) {
            if (is_intersect_poly2(*iter,*s, *S1)){
                core_list_c_.push_back(*iter);
                core_list_un_st_.remove(*iter);
                break;
            }
        }
    }
    if (!Istersection2 && (hole_nb < tmp->id_ || hole_nb == core_num +1)){
        for (list<corePolygon *>::iterator iter = core_list_un_st_.begin(); iter != core_list_un_st_.end(); iter++) {
            if (is_intersect_poly2(*iter, *s, *S2)) {
                core_list_c_.push_back(*iter);
                core_list_un_st_.remove(*iter);
                break;
            }
        }
        for (list<corePolygon *>::iterator iter = core_list_un_st_.begin(); iter != core_list_un_st_.end(); iter++) {
            if (is_intersect_poly2(*iter, *s, *S2)) {
                core_list_c_.push_back(*iter);
                core_list_un_st_.remove(*iter);
                break;
            }
        }

    }
    return;
}
#endif