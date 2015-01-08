/*
 * geo_math_helper.cc
 *
 * 		Last edited on Nov 3, 2013
 * 		by Trong Nguyen
 * 		trongdnguyen@hotmail.com
 *
 */

#include "geo_math_helper.h"
#include "float.h"
#include "stdio.h"

// ------------------------ Geographic ------------------------ //

// check whether 3 points (x1, y1), (x2, y2), (x3, y3) are in same line
bool	G::is_in_line(double x1, double y1, double x2, double y2, double x3, double y3)
{
	double v1x = x2 - x1;
	double v1y = y2 - y1;
	double v2x = x3 - x1;
	double v2y = y3 - y1;

	if (v1x == 0) {
		if (v2x == 0) return true;
		else return false;
	}

	if (v1y == 0) {
		if (v2y == 0) return true;
		else return false;
	}

	if (v2x / v1x == v2y / v1y) return true;
	else return false;
}

// check if x is between a and b
bool G::is_between(double x, double a, double b)
{
	return (x - a) * (x - b) <  0 || (x == a && x == b);	// x c (a;b) V x = a = b
	return (x - a) * (x - b) <= 0;							// x c [a;b] ?
	return (x - a) * (x - b) <  0;							// x c (a;b) ?
	return (x - a) * (x - b) <  0 || (x == a);				// x c [a;b) ?
	return (x - a) * (x - b) <  0 || (x == b);				// x c (a;b] ?
}
// check if (x, y) is between (x1, y1) and (x2, y2)
bool 	G::is_between(double x, double y, double x1, double y1, double x2, double y2)
{
	if (G::is_between(x, x1, x2) && G::is_between(y, y1, y2))
		return true;

	return false;
}

double	G::distance(Point* p1, Point* p2)
{
	return distance(p1->x_, p1->y_, p2->x_, p2->y_);
}
double	G::distance(double x1, double y1, double x2, double y2)
{
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

// distance of p and line l
double	G::distance(Point* p, Line* l)
{
	return distance(p->x_, p->y_, l->a_, l->b_, l->c_);
}
double	G::distance(double x0, double y0, double x1, double y1, double x2, double y2)
{
	double a, b, c;
	line(x1, y1, x2, y2, a, b, c);
	return distance(x0, y0, a, b, c);
}
double	G::distance(double x, double y, double a, double b, double c)
{
	return fabs(a * x + b * y + c) / sqrt(a * a + b * b);
}

// side of p to l. 0 : l contain p, <0 side, >0 other side
int 	G::position(Point* p, Line* l)
{
	double temp = l->a_ * p->x_ + l->b_ * p->y_ + l->c_;
	return temp ? (temp > 0 ? 1 : -1) : 0;
}

// angle of line l with vector ox
Angle	G::angle(Line* l)
{
	// ax + by + c = 0
	// y = -a/b x - c/b
	if (l->b_ == 0) return M_PI/2;
	return atan(-l->a_ / l->b_);
}

/**
 * angle of vector (p1, p2) with Ox - "theta" in polar coordinate
 * return angle between (-Pi, Pi]
 */
Angle 	G::angle(Point p1, Point p2)
{
	return atan2(p2.y_ - p1.y_, p2.x_ - p1.x_);
}

/**
 * absolute value of angle (p1, p0, p2)
 */
Angle	G::angle(Point* p0, Point* p1, Point* p2)
{
	if (*p0 == *p1 || *p0 == *p2) return 0;

	double re = fabs(atan2(p1->y_ - p0->y_, p1->x_ - p0->x_) - atan2(p2->y_ - p0->y_, p2->x_ - p0->x_));
	if (re > M_PI) re = 2 * M_PI - re;
	return re;
}

/**
 * angle of vector (p3, p2) to vector (p1, p0)
 */
Angle	G::angle(Point p0, Point p1, Point p2, Point p3)
{
	double re = atan2(p1.y_ - p0.y_, p1.x_ - p0.x_) - atan2(p3.y_ - p2.y_, p3.x_ - p2.x_);
	return (re < 0) ? re + 2 * M_PI : re;
}

// angle of vector (v2) to vector (v1)
Angle	G::angle(Vector v1, Vector v2)
{
	double re = atan2(v1.b_, v1.a_) - atan2(v2.b_, v2.a_);
	return (re < 0) ? re + 2 * M_PI : re;
}

// angle from segment ((x2, y2), (x0, y0)) to segment ((x1, y1), (x0, y0))
double	G::angle(double x0, double y0, double x1, double y1, double x2, double y2) {
	double a =  atan2(y1 - y0, x1 - x0) - atan2(y2 - y0, x2 - x0);

	return (a < 0) ? a + 2 * M_PI : a;
}

// Check if segment [p1, p2] and segment [p3, p4] are intersected
bool	G::is_intersect(Point* p1, Point* p2, Point* p3, Point* p4)
{
	Line l1 = line(p1, p2);
	Line l2 = line(p3, p4);
	Point in;
	return  (intersection(l1, l2, in)
		&&  ((in.x_ - p1->x_) * (in.x_ - p2->x_) <= 0)
		&& 	((in.y_ - p1->y_) * (in.y_ - p2->y_) <= 0)
		&& 	((in.x_ - p3->x_) * (in.x_ - p4->x_) <= 0)
		&& 	((in.y_ - p3->y_) * (in.y_ - p4->y_) <= 0));
}

// Check if segment (p1, p2) and segment (p3, p4) are intersected
bool	G::is_intersect2(Point* p1, Point* p2, Point* p3, Point* p4)
{
	Line l1 = line(p1, p2);
	Line l2 = line(p3, p4);
	Point in;
	return  (intersection(l1, l2, in)
		&&  ((in.x_ - p1->x_) * (in.x_ - p2->x_) < 0)
		&& 	((in.y_ - p1->y_) * (in.y_ - p2->y_) < 0)
		&& 	((in.x_ - p3->x_) * (in.x_ - p4->x_) < 0)
		&& 	((in.y_ - p3->y_) * (in.y_ - p4->y_) < 0));
}

// Check if (x1, y1)(x2, y2) and (x3, y3)(x4, y4) is intersect
bool 	G::is_intersect(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4)
{
	double x, y;
	if (intersection(x1, y1, x2, y2, x3, y3, x4, y4, x, y))
	{
		if (G::is_between(x, x1, x2) && G::is_between(x, x3, x4) &&
			G::is_between(y, y1, y2) && G::is_between(y, y3, y4))
				return true;
	}
	return false;
}


// Check if segment [p1, p2] and segment [p3, p4] are intersected
bool	G::intersection(Point* p1, Point* p2, Point* p3, Point* p4, Point& p)
{
	Line l1 = line(p1, p2);
	Line l2 = line(p3, p4);
	return (intersection(l1, l2, p)
		&&  ((p.x_ - p1->x_) * (p.x_ - p2->x_) <= 0)
		&& 	((p.y_ - p1->y_) * (p.y_ - p2->y_) <= 0)
		&& 	((p.x_ - p3->x_) * (p.x_ - p4->x_) <= 0)
		&& 	((p.y_ - p3->y_) * (p.y_ - p4->y_) <= 0));
}

// Check if Ellipse e and Line l is intersect
int		G::intersection(Ellipse* e, Line* l, Point & p1, Point & p2)
{
	if (l->a_ == 0)
	{
		double y = - l->c_ / l->b_;
		double a = e->A();
		double b = e->D() + e->B() * y;
		double c = e->C() * y * y + e->E() * y + e->F();

		int n = quadratic_equation(a, b, c, p1.x_, p2.x_);
		if (n) p1.y_ = p2.y_ = y;
		return n;
	}
	else
	{
		double a =     e->A() * l->b_ * l->b_ - e->B() * l->a_ * l->b_ + e->C() * l->a_ * l->a_;
		double b = 2 * e->A() * l->b_ * l->c_ - e->B() * l->a_ * l->c_ - e->D() * l->a_ * l->b_ + e->E() * l->a_ * l->a_;
		double c =     e->A() * l->c_ * l->c_ - e->D() * l->a_ * l->c_ + e->F() * l->a_ * l->a_;

		int n = quadratic_equation(a, b, c, p1.y_, p2.y_);

		if (n)
		{
			p1.x_ = (- l->b_ * p1.y_ - l->c_) / l->a_;
			p2.x_ = (- l->b_ * p2.y_ - l->c_) / l->a_;
		}

		return n;
	}
}

// Point that is intersection point of l1 and l2
bool	G::intersection(Line l1, Line l2, Point& p)
{
	return intersection(l1.a_, l1.b_, l1.c_, l2.a_, l2.b_, l2.c_, p.x_, p.y_);
}
// intersection point
bool 	G::intersection(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double &x, double &y) {
	double a1, b1, c1, a2, b2, c2;

	G::line(x1, y1, x2, y2, a1, b1, c1);
	G::line(x3, y3, x4, y4, a2, b2, c2);

	return intersection(a1, b2, c1, a2, b2, c2, x, y);
}
bool	G::intersection(double a1, double b1, double c1, double a2, double b2, double c2, double &x, double &y) {
	if (a1 == 0 && b1 == 0)
		return false;
	if (a2 == 0 && b2 == 0)
		return false;

	if (a1 == 0 && b2 == 0) {
		x = - c2 / a2;
		y = - c1 / b1;
	}
	else if (a2 == 0 && b1 == 0) {
		x = - c1 / a1;
		y = - c2 / b2;
	}
	else if (a1 * b2 != a2 * b1) {
		x = (b1 * c2 - b2 * c1) / (a1 * b2 - a2 * b1);
		y = (c1 * a2 - c2 * a1) / (a1 * b2 - a2 * b1);
	}
	else return a1 * c2 == a2 * c1;

	return true;
}

// midpoint of p1 and p2
Point	G::midpoint(Point p1, Point p2)
{
	Point re;
	re.x_ = (p1.x_ + p2.x_) / 2;
	re.y_ = (p1.y_ + p2.y_) / 2;
	return re;
}

Vector	G::vector(Point  p1, Point  p2)
{
	Vector v;
	v.a_ = p2.x_ - p1.x_;
	v.b_ = p2.y_ - p1.y_;
	return v;
}

// vector with slope k
Vector	G::vector(Angle k)
{
	k = fmod(k, 2 * M_PI);
	Vector v;
	if (k == M_PI_2)
	{
		v.a_ = 0;
		v.b_ = 1;
	}
	else if (k == 3 * M_PI_2)
	{
		v.a_ = 0;
		v.b_ = -1;
	}
	else if (M_PI_2 < k && k < 3 * M_PI_2)
	{
		v.a_ = -1;
		v.b_ = -tan(k);
	}
	else
	{
		v.a_ = 1;
		v.b_ = tan(k);
	}
	return v;
}

// line throw p and have slope k
Line	G::line(Point  p, Angle k)
{
	Line l;
	if (k == M_PI_2 || k == (3 * M_PI_2))
	{
		l.a_ = 1;
		l.b_ = 0;
		l.c_ = - p.x_;
	}
	else
	{
		l.a_ = - tan(k);
		l.b_ = 1;
		l.c_ = - p.y_ - l.a_ * p.x_;
	}

	return l;
}

// line contains p1 p2
Line	G::line(Point p1, Point p2)
{
	Line re;
	line(p1.x_, p1.y_, p2.x_, p2.y_, re.a_, re.b_, re.c_);
	return re;
}


// line contains p and perpendicular with l
Line	G::perpendicular_line(Point* p, Line* l)
{
	Line re;
	perpendicular_line(p->x_, p->y_, l->a_, l->b_, l->c_, re.a_, re.b_, re.c_);
	return re;
}
// line ax + by + c = 0 that pass through (x0, y0) and perpendicular with line created by (x1, y1) and (x2, y2)
void	G::perpendicular_line(double x0, double y0, double x1, double y1, double x2, double y2, double &a, double &b, double &c)
{
	double a1, b1, c1;
	G::line(x1, y1, x2, y2, a1, b1, c1);

	perpendicular_line(x0, y0, a1, b1, c1, a, b, c);
}
// line contains (x0, y0) and perpendicular with line a0x + b0y + c0 = 0
void	G::perpendicular_line(double x0, double y0, double a0, double b0, double c0, double &a, double &b, double &c)
{
	a = b0;
	b = -a0;
	c = -a * x0 - b * y0;
}

// line parallel with l and have distance to l of d
void	G::parallel_line(Line l, double d, Line& l1, Line& l2)
{
	Point p;		// choose p is contended by l
	if (l.a_ == 0)
	{
		p.x_ = 0;
		p.y_ = - l.c_ / l.b_;
	}
	else
	{
		p.y_ = 0;
		p.x_ = - l.c_ / l.a_;
	}

	l1.a_ = l2.a_ = l.a_;
	l1.b_ = l2.b_ = l.b_;

	l1.c_ = - d * sqrt(l.a_ * l.a_ + l.b_ * l.b_) - l.a_ * p.x_ - l.b_ * p.y_;
	l2.c_ =   d * sqrt(l.a_ * l.a_ + l.b_ * l.b_) - l.a_ * p.x_ - l.b_ * p.y_;
}

// line contains p and parallel with l
Line	G::parallel_line(Point* p, Line* l)
{
	//Line* re = (Line*)malloc(sizeof(Line));
	Line re;
	parallel_line(p->x_, p->y_, l->a_, l->b_, l->c_, re.a_, re.b_, re.c_);
	return re;
}
// line ax + by +c = 0 that pass through (x0, y0) and parallel with line created by (x1, y1) and (x2, y2)
void	G::parallel_line(double x0, double y0, double x1, double y1, double x2, double y2, double &a, double &b, double &c) {
	double a1, b1, c1;
	line(x1, y1, x2, y2, a1, b1, c1);
	parallel_line(x0, y0, a1, b1, c1, a, b, c);
}
// line that contains (x0, y0) and parallel with line a0x + b0y + c0 = 0
void	G::parallel_line(double x0, double y0, double a0, double b0, double c0, double &a, double &b, double &c)
{
	a = a0;
	b = b0;
	c = -a0 * x0 - b0 *y0;
}


// the angle bisector line of (p1 p0 p2)
Line	G::angle_bisector(Point p0, Point p1, Point p2)
{
	Line re;
	angle_bisector(p0.x_, p0.y_, p1.x_, p1.y_, p2.x_, p2.y_, re.a_, re.b_, re.c_);
	return re;
}
void	G::angle_bisector(double x0, double y0, double x1, double y1, double x2, double y2, double &a, double &b, double &c) {
	double a1, b1, c1, a2, b2, c2, a3, b3, c3;

	G::line(x0, y0, x1, y1, a1, b1, c1);
	G::line(x0, y0, x2, y2, a2, b2, c2);
	G::line(x1, y1, x2, y2, a3, b3, c3);

	a = a1 / sqrt(a1 * a1 + b1 * b1) - a2 / sqrt(a2 * a2 + b2 * b2);
	b = b1 / sqrt(a1 * a1 + b1 * b1) - b2 / sqrt(a2 * a2 + b2 * b2);
	c = c1 / sqrt(a1 * a1 + b1 * b1) - c2 / sqrt(a2 * a2 + b2 * b2);

	double x, y;

	if (G::intersection(a, b, c, a3, b3, c3, x, y))
		if (G::is_between(x, x1, x2) && G::is_between(y, y1, y2))
			return;

	a = a1 / sqrt(a1 * a1 + b1 * b1) + a2 / sqrt(a2 * a2 + b2 * b2);
	b = b1 / sqrt(a1 * a1 + b1 * b1) + b2 / sqrt(a2 * a2 + b2 * b2);
	c = c1 / sqrt(a1 * a1 + b1 * b1) + c2 / sqrt(a2 * a2 + b2 * b2);
}


/// tangent lines of ellipse e that have tangent point p
Line	G::tangent(Ellipse* e, Point* p)
{
//	double sin = (e->f2_.y_ - e->f1_.y_) / (2 * e->c());
//	double cos = (e->f2_.x_ - e->f1_.x_) / (2 * e->c());
//
//	Line re;
//	re.a_ = cos * (p->x_ * cos + p->y_ * sin) / (e->a() * e->a()) - sin * (p->x_ * sin + p->y_ * cos) / (e->b() * e->b());
//	re.b_ = sin * (p->x_ * cos + p->y_ * sin) / (e->a() * e->a()) + cos * (p->x_ * sin + p->y_ * cos) / (e->b() * e->b());
//	re.c_ = -1;

	Line re;

	re.a_ = 2 * e->A() * p->x_ + e->B() * p->y_ + e->D();
	re.b_ = 2 * e->C() * p->y_ + e->B() * p->x_ + e->E();
	re.c_ = e->D() * p->x_ + e->E() * p->y_ + 2 * e->F();

	return re;
}

// tangent line of circle c with p
bool	G::tangent(Circle* c, Point* p, Line& l1, Line& l2)
{
	Point t1;
	Point t2;
	if (tangent_point(c, p, t1, t2))
	{
		l1 = line(p, t1);
		l2 = line(p, t2);
		return true;
	}
	else
	{
		return false;
	}
}
// get tangent points of tangent lines of circle that has center O(a, b) and radius r pass through M(x, y)
bool	G::tangent_point(Circle* c, Point* p, Point& t1, Point& t2)
{
	return tangent_point(c->x_, c->y_, c->radius_, p->x_, p->y_, t1.x_, t1.y_, t2.x_, t2.y_);
}
// get tangent points of tangent lines of circle that has center O(a, b) and radius r pass through M(x, y)
bool 	G::tangent_point(double a, double b, double r, double x, double y, double &t1x, double &t1y, double &t2x, double &t2y) {
	double d = (x - a) * (x - a) + (y - b) * (y - b);

	if (r <= 0 || d < r * r)
		return false;

	t1y = (r * r * (y - b) + r * fabs(x - a) * sqrt(d - r * r)) / d + b;
	t2y = (r * r * (y - b) - r * fabs(x - a) * sqrt(d - r * r)) / d + b;

	if (x - a < 0) {
		t1x = (r * r * (x - a) + r * (y - b) * sqrt(d - r * r)) / d + a;
		t2x = (r * r * (x - a) - r * (y - b) * sqrt(d - r * r)) / d + a;
	}
	else {
		t1x = (r * r * (x - a) - r * (y - b) * sqrt(d - r * r)) / d + a;
		t2x = (r * r * (x - a) + r * (y - b) * sqrt(d - r * r)) / d + a;
	}

	return true;
}

Circle	G::circumcenter(Point*p1, Point*p2, Point*p3)
{
	Circle c;
	circumcenter(p1->x_, p1->y_, p2->x_, p2->y_, p3->x_, p3->y_, c.x_, c.y_);
	c.radius_ = distance(c, p1);
	return c;
}
// find circumcenter (xo, yo)
void	G::circumcenter(double x1, double y1, double x2, double y2, double x3, double y3, double &xo, double &yo) {
	double a1, b1, c1, a2, b2, c2;

	if (!G::is_in_line(x1, y1, x2, y2, x3, y3)) {
		G::perpendicular_bisector(x1, y1, x2, y2, a1, b1, c1);
		G::perpendicular_bisector(x1, y1, x3, y3, a2, b2, c2);
		if (b1 * a2 == b2 * a1) {
			xo = x1;
			yo = y1;
		} else {
			xo = - (c2 * b1 - c1 * b2) / (b1 * a2 - b2 * a1);
			yo = - (c2 * a1 - c1 * a2) / (a1 * b2 - a2 * b1);
		}
	} else {
		xo = x1;
		yo = y1;
	}
}

// quadratic equation. return number of experiment
int		G::quadratic_equation(double a, double b, double c, double &x1, double &x2)
{
	if (a == 0)	return 0;

	double delta = b * b - 4 * a * c;
	if (delta < 0) return 0;

	x1 = (- b + sqrt(delta)) / (2 * a);
	x2 = (- b - sqrt(delta)) / (2 * a);
	return delta > 0 ? 2 : 1;
}

// find perpendicular bisector of segment ((x1, y1), (x2, y2))
void G::perpendicular_bisector(double x1, double y1, double x2, double y2, double &a, double &b, double &c) {
	a = x1 - x2;
	b = y1 - y2;
	c = ((x2) * (x2) + (y2) * (y2) - x1 * x1 - y1 * y1) / 2;
}

void G::line(double x1, double y1, double x2, double y2, double &a, double &b, double &c) {
	a = y1 - y2;
	b = x2 - x1;
	c = - y1*x2 + y2*x1;
}

double G::area(double ax, double ay, double bx, double by, double cx, double cy) {
	double a, b, c;
	a = G::distance(bx, by, cx, cy);
	b = G::distance(cx, cy, ax, ay);
	c = G::distance(ax, ay, bx, by);

	return sqrt((a + b + c) * (b + c - a) * (c + a - b) * (a + b - c) / 16);
}

// check whether polygon denoted by list of node, head n, is CWW
bool G::is_clockwise(node* n)
{
	node* up = n;
	node* botton = n;
	node* left = n;
	node* right = n;

	node* i = n;
	do {
		if (i->y_ > up->y_)		up = i;
		if (i->y_ < botton->y_) botton = i;
		if (i->x_ < left->x_ )	left = i;
		if (i->x_ > right->x_)	right = i;

		i = i->next_;
	} while (i && i != n);

	i = botton ->next_;
	bool cw = true;
	while (true)
	{
		if (i == left) return cw;
		if (i == right) return !cw;
		if (i == up) cw = !cw;

		i = i->next_;
		if (i == NULL) i = n;
	}

	return i == right;
}

double	G::area(node* n)
{
	node* g1 = NULL;
	node* g2 = NULL;
	node* g3 = NULL;

	// copy polygon
	g1 = new node();
	g1->x_ = n->x_;
	g1->y_ = n->y_;
	g1->next_ = NULL;

	g3 = g1;	// tail of list

	for (node* i = n->next_;i && i != n; i = i->next_)
	{
		node* g2 = new node();
		g2->x_ = i->x_;
		g2->y_ = i->y_;
		g2->next_ = g1;
		g1 = g2;
	}

	// circle node list
	g3->next_ = g1;

	// ------------ calculate area in copy polygon

	bool cww = is_clockwise(g1);
	double s = 0;

	while (g1->next_->next_->next_ != g1)
	{
		while (true) {
			g2 = g1->next_;
			g3 = g2->next_;
			// check whether g1, g3 is inside polygon
			if ((g1 && G::angle(g2, g1, g2, g3) < M_PI) != cww)
			{
				Line l1 = G::line(g2, g3);
				Line l2 = G::line(g3, g1);
				Line l3 = G::line(g1, g2);
				int si1 = G::position(g1, l1);
				int si2 = G::position(g2, l2);
				int si3 = G::position(g3, l3);

				bool ok = true;
				for (node* i = g3->next_; ok && i != g1; i = i->next_)
				{
					ok = (G::position(i, l1) != si1) || (G::position(i, l2) != si2) || (G::position(i, l3) != si3);
				}
				if (ok) break;
			}

			// check whether g1, g2, g3 is in line
			if (G::is_in_line(g1, g2, g3)) break;

			g1 = g1->next_;
		};

		s += area(g1, g2, g3);
		g1->next_ = g3;

		FILE *fp = fopen("Test.tr", "w");
		node* t = g1;
		do {
			fprintf(fp, "%f	%f\n", t->x_, t->y_);
			t = t->next_;
		} while (t && t != g1);

		fprintf(fp, "%f	%f\n\n", g1->x_, g1->y_);
		fclose(fp);

		delete g2;
	}
	s += area(g1, g1->next_, g1->next_->next_);
	delete g1->next_->next_;
	delete g1->next_;
	delete g1;

	return s;
}

