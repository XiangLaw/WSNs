#ifndef BOUNDHOLE_ROUTING_H
#define BOUNDHOLE_ROUTING_H

#include <wsn/boundhole/boundhole.h>


class BOUNDHOLEROUTINGAgent : public BoundHoleAgent {
private:
    void createHole(Packet *p);

    void sendData(Packet *p);

    void recvData(Packet *p);

    polygonHole *hole_list_;

    node* getNextHopByBoundHole(Point);

public:
    BOUNDHOLEROUTINGAgent();

    int command(int, const char *const *);

    void recv(Packet *, Handler *);
};

#endif