

#ifndef GRIDDYNAMIC_H_
#define GRIDDYNAMIC_H_

#include "wsn/geomathhelper/geo_math_helper.h"
#include "wsn/gpsr/gpsr.h"
#include "wsn/gridoffline/grid_packet.h"

class GridDynamicAgent;

struct stuckangle
{
	// two neighbors that create stuck angle with node.
	node *a_;
	node *b_;

	stuckangle *next_;
};

int limit_boundhole_hop_;
double nodeoff_threshold_; // evaluate by time
double alert_threshold_; // evaluate by percent
double sink_threshold_; // evaluate by percent
double limit_x_;
double limit_y_;
double r_;
double range_;
double limit_;

struct gridHole {
	int hole_id_;
	bool** a_;
	gridHole* next_;
	struct node* node_list_;

	~gridHole()
	{
		clearNodeList();

		if (a_ != NULL)
		{
			for (int i = 0; i < limit_x_; i++)
				delete a_[i];
			delete a_;
		}
	}

	void circleNodeList()
	{
		node* temp = node_list_;
		while (temp->next_ && temp->next_ != node_list_) temp = temp->next_;
		temp->next_ = node_list_;
	}

	void clearNodeList()
	{
		if (node_list_ != NULL)
		{
			node* temp = node_list_;
			do {
				delete temp;
				temp = temp->next_;
			} while(temp && temp != node_list_);
			node_list_ = NULL;
		}
	}

	void dump()
	{
		int nx = ceil(limit_x_ / r_);
		int ny = ceil(limit_y_ / r_);

		FILE *fp = fopen("GridHole.tr", "a+");
		for (int j = ny - 1; j >= 0; j--)
		{
			for (int i = 0; i < nx; i++)
			{
				fprintf(fp, "%d ", a_[i][j]);
			}
			fprintf(fp, "\n");
		}
		fprintf(fp,"\n");
		fclose(fp);
	}
};

typedef void(GridDynamicAgent::*firefunction)(void);

class GridDynamicTimer : public TimerHandler
{
	public:
		GridDynamicTimer(GridDynamicAgent *a, firefunction f) : TimerHandler() {
			a_ = a;
			firing_ = f;
		}

	protected:
		virtual void expire(Event *e);
		GridDynamicAgent *a_;
		firefunction firing_;
};

class GridDynamicAgent : public GPSRAgent {
private:
	friend class GridDynamicHelloTimer;
	GridDynamicTimer findStuck_timer_;
	GridDynamicTimer updateSta_timer_;

	stuckangle* stuck_angle_;
	gridHole* hole_;
    neighbor* tracking_list_; // current nodes in tracking by pivot
    node* max_node_list_; // storage node in cell by pivot

	bool isBoundary;
	bool isSink;
	bool isStuck;
    bool isAlerted;
    bool isSinkNotify;
    bool forceFindStuck;
	node pivot;
	int nx_, ny_;

	void startUp();

	bool removeNodeoff();
    bool removeTrackingNodeOff();
	void findStuckAngle();
	node* getNeighborByBoundhole(Point*, Point*);
    void addTracking(nsaddr_t saddr, Point point);

	void sendBoundHole();
	void recvBoundHole(Packet*);

	void addData(Packet*);
	bool checkCell(Point point);

	void createGridHole(Packet*, Point);
	bool updateGridHole(Packet*, gridHole*);

	void createPolygonHole(gridHole*);
	void reducePolygonHole(gridHole*);

	void updateState();
	void checkState();

	void recvGridDynamic(Packet* p);

	void sendNotify();
	void recvNotify(Packet *);

	void sendPivot();
	void recvPivot(Packet *);

	void sendAlarm();

	void sendCollect(Packet*);
	void recvCollect(Packet*);

	void sendUpdate();
	void recvUpdate(Packet*);

	void dumpBoundhole(gridHole*);
	void dumpPivot();
	void dumpCollect();
	void dumpAlarm();
	void dumpNodeOff(neighbor*);
    void dumpNodeOffReal();

public:
	GridDynamicAgent();
	int 	command(int, const char*const*);
	void 	recv(Packet*, Handler*);

	static node* sink_list_;

    void dumpAdjacentHole(node *p);
};

#endif /* GRID_H_ */
