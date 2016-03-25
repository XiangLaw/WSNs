/*
 * geo_math_helper.cc
 *
 * 		Last edited on Nov 3, 2013
 * 		by Trong Nguyen
 * 		trongdnguyen@hotmail.com
 *		http://www.ltcconline.net/greenl/courses/107/vectors/dotcros.htm
 */

#include "geo_math_helper.h"
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

// side of p1 to p2 through l: >0 if same side, =0 if either p1 or p2 stay on l1, <0 if difference side
int G::position(Point *p1, Point *p2, Line *l) {
    int pos1 = G::position(p1, l);
    int pos2 = G::position(p2, l);

    return pos1*pos2;
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
 * angle of vector (p2, p3) to vector (p0, p1)
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

		delete g2;
	}
	s += area(g1, g1->next_, g1->next_->next_);
	delete g1->next_->next_;
	delete g1->next_;
	delete g1;

	return s;
}



/*
 * Extension function
 */

/**
* directed angle (pb, pa) in clockwise (between [-180;180))
* */
Angle G::directedAngle(Point* b, Point* p, Point* a)
{
//    if (*a == *p || *a == *b) return 0;
//    double alpha = (atan2(a->y_ - p->y_, a->x_ - p->x_) - atan2(b->y_ - p->y_, b->x_ - p->x_));
//    return alpha;
//    // reduce if |alpha| > PI
//    if (alpha > M_PI){
//        alpha -= M_PI;
//    } else if (alpha < -M_PI){
//        alpha += M_PI;
//    }
//    return alpha;
    double x1, y1, x2, y2;
    x1 = a->x_ - p->x_;
    x2 = b->x_ - p->x_;
    y1 = a->y_ - p->y_;
    y2 = b->y_ - p->y_;
    double dot = x1*x2 + y1*y2;      // dot product
    double det = x1*y2 - y1*x2;      // determinant
    double angle = atan2(det, dot);  // atan2(y, x) or atan2(sin, cos)

    return angle;
}

bool G::lineSegmentIntersection(Point *a, Point *b, Line l, Point &intersection) {
    intersection.x_ = -1;
    intersection.y_ = -1;
    Line edge = G::line(a, b);

    if (G::intersection(edge, l, &intersection) && (intersection.x_ >= 0 && intersection.y_ >= 0)) {
        if(
                (intersection.y_ - a->y_) * (intersection.y_ - b->y_) <= 0)
            return true;
    }
    return false;
}

// see http://mathworld.wolfram.com/Circle-CircleIntersection.html
// p1: left side of vector (c1, c2)
// p2: right side of vector (c1, c2)
int G::circleCircleIntersect(Point c1, double r1, Point c2, double r2, Point *p1, Point *p2)
{
    int nsoln = -1;

    Point c;
    c.x_ = c2.x_ - c1.x_;
    c.y_ = c2.y_ - c1.y_;

    nsoln = G::circleCircleIntersect0a( r1, c, r2, p1, p2 );
    /* Translate back. */
    p1->x_ += c1.x_;
    p1->y_ += c1.y_;
    p2->x_ += c1.x_;
    p2->y_ += c1.y_;
    return nsoln;
}

/*---------------------------------------------------------------------
circleCircleIntersect0a assumes that the first circle is centered on the origin.
Returns # of intersections: 0, 1, 2, 3 (inf); point in p.
---------------------------------------------------------------------*/
int G::circleCircleIntersect0a(double r1, Point c2, double r2, Point* p1, Point *p2 )
{
    double dc2;              /* dist to center 2 squared */
    double rplus2, rminus2;  /* (r1 +/- r2)^2 */
    double f;                /* fraction along c2 for nsoln=1 */

    /* Handle special cases. */
    dc2 = c2.x_*c2.x_ + c2.y_*c2.y_;
    rplus2  = (r1 + r2) * (r1 + r2);
    rminus2 = (r1 - r2) * (r1 - r2);

    /* No solution if c2 out of reach + or -. */
    if ( ( dc2 > rplus2 ) || ( dc2 < rminus2 ) )
        return   0;

    /* One solution if c2 just reached. */
    /* Then solution is r1-of-the-way (f) to c2. */
    if ( dc2 == rplus2 ) {
        f = r1 / (double)(r1 + r2);
        p1->x_ = p2->x_ = f*c2.x_;
        p1->y_ = p2->y_ = f*c2.y_;
        return 1;
    }
    if ( dc2 == rminus2 ) {
        if ( rminus2 == 0 ) {   /* Circles coincide. */
            p1->x_ = p2->x_ = r1;
            p1->y_ = p2->y_ = 0;
            return 3; // infinite
        }
        f = r1 / (double)(r1 - r2);
        p1->x_ = p2->x_ = f * c2.x_;
        p1->y_ = p2->y_ = f * c2.y_;
        return 1;
    }

    /* Two intersections. */
    return G::circleCircleIntersect0b( r1, c2, r2, p1, p2);
}

/*---------------------------------------------------------------------
circleCircleIntersect0b also assumes that the 1st circle is origin-centered.
---------------------------------------------------------------------*/
int  G::circleCircleIntersect0b( double r1, Point c2, double r2, Point *p1, Point *p2)
{
    double a2;          /* center of 2nd circle when rotated to x-axis */
    Point q1, q2;          /* solution when c2 on x-axis */
    double cost, sint;  /* sine and cosine of angle of c2 */

    /* Rotate c2 to a2 on x-axis. */
    a2 = sqrt( c2.x_*c2.x_ + c2.y_*c2.y_ );
    cost = c2.x_ / a2;
    sint = c2.y_ / a2;

    G::circleCircleIntersect00( r1, a2, r2, &q1, &q2);

    /* Rotate back */
    p1->x_ =  cost * q1.x_ + -sint * q1.y_;
    p1->y_ =  sint * q1.x_ +  cost * q1.y_;
    p2->x_ =  cost * q2.x_ + -sint * q2.y_;
    p2->y_ =  sint * q2.x_ +  cost * q2.y_;

    return 2;
}

/*---------------------------------------------------------------------
circleCircleIntersect00 assumes circle centers are (0,0) and (a2,0).
---------------------------------------------------------------------*/
void  G::circleCircleIntersect00( double r1, double a2, double r2, Point* p1, Point *p2 )
{
    double r1sq, r2sq;
    r1sq = r1*r1;
    r2sq = r2*r2;

    /* Return only positive-y soln in p. */
    p1->x_ = p2->x_ = ( a2 + ( r1sq - r2sq ) / a2 ) / 2;
    p1->y_ = sqrt( r1sq - p1->x_*p1->x_ );
    p2->y_ = -p1->y_;
}

// see at http://mathworld.wolfram.com/Circle-LineIntersection.html
// return intersection of (c,r) with line (i1, i2)
int G::circleLineIntersect(Point c, double r, Point i1, Point i2, Point *p1, Point *p2){
    i1.x_ = i1.x_ - c.x_;
    i2.x_ = i2.x_ - c.x_;
    i1.y_ = i1.y_ - c.y_;
    i2.y_ = i2.y_ - c.y_;

    int re = circleLineIntersect00(r, i1, i2, p1, p2);
    p1->x_ += c.x_;
    p2->x_ += c.x_;
    p1->y_ += c.y_;
    p2->y_ += c.y_;
    return re;
}

// intersect of line l and circle ((0,0), r)
int G::circleLineIntersect00(double r, Point i1, Point i2, Point *p1, Point *p2) {
    double dx = i2.x_ - i1.x_;
    double dy = i2.y_ - i1.y_;
    double dr = sqrt(dx*dx + dy*dy);
    double d =  i1.x_*i2.y_ - i1.y_*i2.x_;
    double delta = r*r*dr*dr - d*d;

    if (delta < 0){
        return 0;
    } else if (delta == 0){
        p1->x_ = p2->x_ = d*dy/(dr*dr);
        p1->y_ = p2->y_ = -d*dx/(dr*dr);
        return 1;
    } else {
        int sgn = dy < 0 ? -1 : 1;
        p1->x_ = (d * dy + sgn * dx * sqrt(delta)) / (dr * dr);
        p2->x_ = (d * dy - sgn * dx * sqrt(delta)) / (dr * dr);
        p1->y_ = (-d * dx + fabs(dy) * sqrt(delta)) / (dr * dr);
        p2->y_ = (-d * dx - fabs(dy) * sqrt(delta)) / (dr * dr);
        return 2;
    }
}


// clockwise directed angle between vector (pa, px // Ox) in range [-180, 180]
Angle G::angle_x_axis(Point *a, Point *p) {
	double x1, y1;
	x1 = a->x_ - p->x_;
	y1 = a->y_ - p->y_;

	double a1 = atan2(y1, x1);
	return a1;
}

// Given three colinear points p, q, r, the function checks if
// point q lies on line segment 'pr'
bool G::onSegment(Point p, Point q, Point r)
{
	return q.x_ <= g_max(p.x_, r.x_) && q.x_ >= g_min(p.x_, r.x_) &&
    q.y_ <= g_max(p.y_, r.y_) && q.y_ >= g_min(p.y_, r.y_);

}

// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are colinear
// 1 --> Clockwise
// 2 --> Counterclockwise
int G::orientation(Point p, Point q, Point r)
{
	// See 10th slides from following link for derivation of the formula
	// http://www.dcs.gla.ac.uk/~pat/52233/slides/Geometry1x1.pdf
	double val = (q.y_ - p.y_) * (r.x_ - q.x_) -
			  (q.x_ - p.x_) * (r.y_ - q.y_);

	if (val == 0) return 0; // colinear

	return (val > 0)? 1: 2; // clock or counterclock wise
}

// The main function that returns true if line segment 'p1q1'
// and 'p2q2' intersect.
bool G::doIntersect(Point p1, Point q1, Point p2, Point q2)
{
	// Find the four orientations needed for general and
	// special cases
	int o1 = orientation(p1, q1, p2);
	int o2 = orientation(p1, q1, q2);
	int o3 = orientation(p2, q2, p1);
	int o4 = orientation(p2, q2, q1);

	// General case
	if (o1 != o2 && o3 != o4)
		return true;

	// Special Cases
	// p1, q1 and p2 are colinear and p2 lies on segment p1q1
	if (o1 == 0 && onSegment(p1, p2, q1)) return true;

	// p1, q1 and p2 are colinear and q2 lies on segment p1q1
	if (o2 == 0 && onSegment(p1, q2, q1)) return true;

	// p2, q2 and p1 are colinear and p1 lies on segment p2q2
	if (o3 == 0 && onSegment(p2, p1, q2)) return true;

	// p2, q2 and q1 are colinear and q1 lies on segment p2q2
	if (o4 == 0 && onSegment(p2, q1, q2)) return true;

	return false; // Doesn't fall in any of the above cases
}

/**
 * Check if a Point is inside (including lies on edge) of a grid
 */
bool G::isPointInsidePolygon(Point *d, node *node_list) {
    node *tmp;
    bool odd = false;

    // count horizontal
    for (tmp = node_list; tmp != NULL; tmp = tmp->next_) {
        if (tmp->next_ != NULL) {
            if (G::is_in_line(tmp, tmp->next_, d)) {
                if (G::onSegment(tmp, d, tmp->next_)) {
                    return true;
                }
            }
            if (((tmp->y_ < d->y_ && tmp->next_->y_ >= d->y_) ||
                 (tmp->next_->y_ < d->y_ && tmp->y_ >= d->y_)) &&
                (tmp->x_ <= d->x_ && tmp->next_->x_ <= d->x_)) {
                double denominator = (tmp->next_->y_ - tmp->y_) * (tmp->next_->x_ - tmp->x_);
                if(denominator != 0) {
                    if(tmp->x_ + (d->y_ - tmp->y_) / denominator < d->x_) {
                        odd = !odd;
                    }
                }
            }
        }
        else { // end-point & start-point
            if (G::is_in_line(tmp, node_list, d)) {
                if (G::onSegment(tmp, d, node_list)) {
                    return true;
                }
            }
            if (((tmp->y_ < d->y_ && node_list->y_ >= d->y_) ||
                 (node_list->y_ < d->y_ && tmp->y_ >= d->y_)) &&
                (tmp->x_ <= d->x_ && node_list->x_ <= d->x_)) {
                double denominator = (node_list->y_ - tmp->y_) * (node_list->x_ - tmp->x_);
                if(denominator != 0) {
                    if(tmp->x_ + (d->y_ - tmp->y_) / denominator < d->x_) {
                        odd = !odd;
                    }
                }
            }
        }
    }

    return odd;
}

/**
 * Check if point is inside P1P2P3 triangle
 */
bool G::isPointLiesInTriangle(Point *p, Point *p1, Point *p2, Point *p3) {
	// barycentric algorithm
	double alpha = ((p2->y_ - p3->y_) * (p->x_ - p3->x_) + (p3->x_ - p2->x_) * (p->y_ - p3->y_)) /
				   ((p2->y_ - p3->y_) * (p1->x_ - p3->x_) + (p3->x_ - p2->x_) * (p1->y_ - p3->y_));
	double beta = ((p3->y_ - p1->y_) * (p->x_ - p3->x_) + (p1->x_ - p3->x_) * (p->y_ - p3->y_)) /
				  ((p2->y_ - p3->y_) * (p1->x_ - p3->x_) + (p3->x_ - p2->x_) * (p1->y_ - p3->y_));
	double gamma = 1.0f - alpha - beta;

	return gamma >= 0 && alpha >= 0 && beta >= 0;
}

bool G::isPointReallyInsidePolygon(Point *d, node *node_list) {
    node *tmp;
    bool odd = false;

    // count horizontal
    for (tmp = node_list; tmp != NULL; tmp = tmp->next_) {
        if (tmp->next_ != NULL) {
            if (G::is_in_line(tmp, tmp->next_, d)) {
                if (G::onSegment(tmp, d, tmp->next_)) {
                    return false;
                }
            }
            if (((tmp->y_ < d->y_ && tmp->next_->y_ >= d->y_) ||
                 (tmp->next_->y_ < d->y_ && tmp->y_ >= d->y_)) &&
                (tmp->x_ <= d->x_ && tmp->next_->x_ <= d->x_)) {
                double denominator = (tmp->next_->y_ - tmp->y_) * (tmp->next_->x_ - tmp->x_);
                if(denominator != 0) {
                    if(tmp->x_ + (d->y_ - tmp->y_) / denominator < d->x_) {
                        odd = !odd;
                    }
                }
            }
        }
        else { // end-point & start-point
            if (G::is_in_line(tmp, node_list, d)) {
                if (G::onSegment(tmp, d, node_list)) {
                    return false;
                }
            }
            if (((tmp->y_ < d->y_ && node_list->y_ >= d->y_) ||
                 (node_list->y_ < d->y_ && tmp->y_ >= d->y_)) &&
                (tmp->x_ <= d->x_ && node_list->x_ <= d->x_)) {
                double denominator = (node_list->y_ - tmp->y_) * (node_list->x_ - tmp->x_);
                if(denominator != 0) {
                    if(tmp->x_ + (d->y_ - tmp->y_) / denominator < d->x_) {
                        odd = !odd;
                    }
                }
            }
        }
    }

    return odd;
}

// get aggregation between segment [a1, a2] and [b1, b2]
// return to a1, a2
int G::segmentAggregation(Point *a1, Point *a2, Point *b1, Point *b2) {
    Point tmp;
    if (a1->x_ > a2->x_ || (a1->x_ == a2->y_ && a1->y_ > a2->y_)){
        tmp = *a1;
        *a1 = *a2;
        *a2 = tmp;
    }

    if (b1->x_ > b2->x_ || (b1->x_ == b2->y_ && b1->y_ > b2->y_)){
        tmp = *b1;
        *b1 = *b2;
        *b2 = tmp;
    }

    if (onSegment(a1, b1, a2)){
        *a1 = *b1;
        if (onSegment(a1, b2, a2)){
            *a2 = * b2;
        }
        return 1;
    } else {
        if (onSegment(a1, b2, a2)){
            *a2 = *b2;
            return 1;
        } else {
            if (onSegment(b1, a1, b2)){
                return 1;
            }
        }
    }

    return 0;
}

int G::orientation(node *p, Point q, node *r) {
	Point p1, r1;
	p1.x_ = p->x_;
	p1.y_ = p->y_;
	r1.x_ = r->x_;
	r1.y_ = r->y_;
	return orientation(p1, q, r1);
}
