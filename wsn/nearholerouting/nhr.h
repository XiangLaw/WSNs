#ifndef NHR_H
#define NHR_H

#include <wsn/boundhole/boundhole.h>

class NHRAgent : public BoundHoleAgent
{
private:
    polygonHole *hole_list_;

    void routing();
    void startUp();

    void createHole(Packet *p);
    void sendData(Packet* p);
    void recvData(Packet* p);
public:
    NHRAgent();
    int command(int, const char*const*);
    void recv(Packet*, Handler*);
};
#endif