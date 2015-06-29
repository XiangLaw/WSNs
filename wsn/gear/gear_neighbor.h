#ifndef GNEIGHBOR_H_
#define GNEIGHBOR_H_

#include "packet.h"

#define DEFAULT_TIMEOUT 200.0
        //the default time out period is 200.0 sec
        //If the hello messge was not received during this period
        //the entry in the neighbor list may be deleted

#define CURRENT Scheduler::instance().clock()
#define INFINITE_DELAY 5000000000000.0

#define INFINITE_DISTANCE 100000000000.0

struct gneighbor {
  nsaddr_t id_;

  // the geographic location
  double x_;
  double y_;

  double e_;

  double h_;

  // the last timestamp for the hello message from it
  double ts_;

  // pointer to next and previous neighbor
  gneighbor *next_;
  gneighbor *prev_;
};

class GNeighbors {
private:
	gneighbor *head_;		// the neighbors list
	gneighbor *tail_;

	int nbSize_;				// the neighbors list size

	nsaddr_t my_id_; 				// my id
	double my_x_;    				// my geo info
	double my_y_;

	// free neighbors
	void free_neighbors(struct gneighbor*);

public:
	GNeighbors();
	~GNeighbors();

	// update location information for myself
	void myinfo(nsaddr_t, double, double);

	// return neighbors list size
	int nbSize();

	// add a possible new neighbor
	void newnb(nsaddr_t, double, double, double);

	//find the entry in neighbor list according to the provided id
	struct gneighbor *getnb(nsaddr_t);

	struct gneighbor *getnb_at(int index);

	//delete the entry in neighbors list according to the provided id
	void delnb(nsaddr_t);

	//delete the entry directly
	void delnb(struct gneighbor *);

	void dump(FILE *);
};

#endif
