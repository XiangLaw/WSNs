#ifndef STRUCT_H_
#define STRUCT_H_

struct stuckangle
{
    // two neighbors that create stuck angle with node.
    node *a_;
    node *b_;

    stuckangle *next_;
};


struct polygonHole
{
    int hole_id_;
    struct node* node_list_;
    struct polygonHole* next_;

    ~polygonHole()
    {
        node* temp = node_list_;
        do {
            delete temp;
            temp = temp->next_;
        } while(temp && temp != node_list_);
    }

    void circleNodeList()
    {
        node* temp = node_list_;
        while (temp->next_ && temp->next_ != node_list_) temp = temp->next_;
        temp->next_ = node_list_;
    }

    void unCircleNodeList(){
        node* temp = node_list_;
        do {temp = temp->next_;}
        while (temp->next_ && temp->next_ != node_list_);
        temp->next_ = NULL;
    }
};

struct corePolygon
{
    int id_;
    struct node* node_;
    struct corePolygon* next_;

    ~corePolygon() {
        node *tmp = node_;
        do {
            node_ = node_->next_;
            delete tmp;
        } while (tmp && tmp != node_);
    }

    void circleNodeList()
    {
        node* temp = node_;
        while (temp->next_ && temp->next_ != node_) temp = temp->next_;
        temp->next_ = node_;
    }

    void unCircleNodeList(){
        node* temp = node_;
        do {temp = temp->next_;}
        while (temp->next_ && temp->next_ != node_);
        temp->next_ = NULL;
    }
};

#endif