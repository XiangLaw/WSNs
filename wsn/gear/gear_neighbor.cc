/*
 * neighbor.cc
 *
 *  Created on: May 26, 2011
 *      Author: leecom
 */

#include "gear_neighbor.h"
#include "math.h"

#include "wsn/geomathhelper/geo_math_helper.h"

#define MAX(a, b) (a >= b ? a : b)
#define MIN(a, b) (a >= b ? b : a)
#define PI 3.14159

GNeighbors::GNeighbors() {
	my_id_ = -1;
	my_x_ = -1;
	my_y_ = -1;

	head_ = NULL;
	nbSize_ = 0;
}

GNeighbors::~GNeighbors() {
	struct gneighbor *temp = head_;
	while (temp) {
		temp = temp->next_;
		free(head_);
		head_ = temp;
	}
}

int GNeighbors::nbSize(){
  return nbSize_;
}

void GNeighbors::myinfo(nsaddr_t mid, double mx, double my){
  my_id_ = mid;
  my_x_ = mx;
  my_y_ = my;
}

struct gneighbor* GNeighbors::getnb(nsaddr_t nid){
	struct gneighbor *temp = head_;
	while(temp){
		if(temp->id_ == nid){
			/*
			if((CURRENT - temp->ts_) < DEFAULT_TIMEOUT)
				return temp;
			else {
				delnb(temp); //if this entry expire, delete it and return NULL
				return NULL;
			}*/
			return temp;
		}
		temp = temp->next_;
	}
	return NULL;
}

struct gneighbor* GNeighbors::getnb_at(int index) {
	int i = 0;
	struct gneighbor *temp = head_;

	while (temp) {
		if (i == index) {
			return temp;
		}
		i++;
		temp = temp->next_;
	}

	return NULL;
}

void GNeighbors::newnb(nsaddr_t nid, double nx, double ny, double ne){
	gneighbor *temp = getnb(nid);

	if(temp==NULL){ //it is a new neighbor
		temp=(gneighbor*)malloc(sizeof(gneighbor));
		temp->id_ = nid;
		temp->x_ = nx;
		temp->y_ = ny;
		temp->e_ = ne;
		temp->h_ = -1;
		temp->ts_ = CURRENT;
		temp->next_ = NULL;
		temp->prev_ = NULL;

		if(head_ == NULL){ //the list now is empty
			head_ = temp;
			tail_ = temp;
			nbSize_ = 1;
		}
		else { //now the neighbors list is not empty
			tail_->next_ = temp;
			temp->prev_ = tail_;
			tail_ = temp;
			nbSize_++;
		}
	}
	else { //it is a already known neighbor
		temp->ts_ = CURRENT;
		temp->x_ = nx; //the updating of location is allowed
		temp->y_ = ny;
		temp->e_ = ne;
		temp->h_ = -1;
	}
}

void GNeighbors::delnb(nsaddr_t nid){
	struct gneighbor *temp = getnb(nid);
	if(temp==NULL) return;
	else delnb(temp);
}

void GNeighbors::delnb(struct gneighbor *nb){
	struct gneighbor *preffix = nb->prev_;

	if(preffix == NULL){
		head_ = nb->next_;
		nb->prev_ = NULL;

		if(head_ == NULL)
		  tail_ = NULL;
		else head_->prev_ = NULL;

		free(nb);
	}
	else {
		preffix->next_ = nb->next_;
		nb->prev_ = NULL;
		if(preffix->next_ == NULL)
		  tail_ = preffix;
		else (preffix->next_)->prev_ = preffix;
		free(nb);
	}

	nbSize_--;
}

void GNeighbors::free_neighbors(struct gneighbor *nblist){
	struct gneighbor *temp, *head;
	head = nblist;
	while(head){
	temp = head;
	head = head->next_;
	free(temp);
	}
}

void GNeighbors::dump(FILE *fp) {
	struct gneighbor *temp = head_;
	while(temp){
		fprintf(fp, "%d,", temp->id_);
		temp = temp->next_;
	}
	fprintf(fp,"\n");
}
