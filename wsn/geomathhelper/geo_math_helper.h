/*
 * geo_math_helper.h
 *
 *  Last Edited on Nov, 2013
 *  By Trong Nguyen
 *
 *	trongdnguyen@hotmail.com
 */

#ifndef GEO_MATH_HELPER_H_
#define GEO_MATH_HELPER_H_

#include "math.h"
#include "cstdlib"
#include "float.h"

typedef double Angle;

class G;

struct Point
{
	double x_;
	double y_;

	inline bool operator==(const Point& rhs) { return x_ == rhs.x_ && y_ == rhs.y_; }
	inline bool operator!=(const Point& rhs) { return !operator==(rhs); }
};

struct node : Point
{
	int32_t id_;
	node* next_;
};

struct Vector
{
	double a_;
	double b_;
};

struct Line
{
	double a_;
	double b_;
	double c_;
};

struct Circle : Point
{
	double radius_;
};

struct Ellipse
{
private:
	double _a, _b, _c, _d, _e, _f;
	double b_, c_;

public:
	Point f1_, f2_;
	double a_;

	Ellipse()
	{
		_a = DBL_MIN;
		_b = DBL_MIN;
		_c = DBL_MIN;
		_d = DBL_MIN;
		_e = DBL_MIN;
		_f = DBL_MIN;
		a_ = DBL_MIN;
		b_ = DBL_MIN;
		c_ = DBL_MIN;
	}

	double &a() { return a_; }
	double b() { return b_ != DBL_MIN ? b_ : b_ = sqrt(a() * a() + c() * c()); }
	double c() { return c_ != DBL_MIN ? c_ : c_ = sqrt((f1_.x_ - f2_.x_) * (f1_.x_ - f2_.x_) + (f1_.y_ - f2_.y_) * (f1_.y_ - f2_.y_)) / 2; }

//	double A() { return _a != DBL_MIN ? _a : _a =  (f2_.x_ - f1_.x_) * (f1_.x_ - f2_.x_) / 2 + a() * a(); }
//	double B() { return _b != DBL_MIN ? _b : _b =  (f2_.y_ - f1_.y_) * (f1_.x_ - f2_.x_); }
//	double C() { return _c != DBL_MIN ? _c : _c =  (f2_.y_ - f1_.y_) * (f1_.y_ - f2_.y_) / 2 + a() * a(); }
//	double D() { return _d != DBL_MIN ? _d : _d = -(f2_.x_ + f1_.x_) * A() - (f1_.y_ + f2_.y_) / 2 * B(); }
//	double E() { return _e != DBL_MIN ? _e : _e = -(f2_.y_ + f1_.y_) * C() - (f1_.x_ + f2_.x_) / 2 * B(); }
//	double F() { return _f != DBL_MIN ? _f : _f =  (f2_.x_ + f1_.x_) * (f1_.x_ + f2_.x_) / 4 * A() + (f1_.y_ + f2_.y_) * (f1_.y_ + f2_.y_) / 4 * C() + (f1_.x_ + f2_.x_) * (f1_.y_ + f2_.y_) / 4 * B() - 1; }

	double A() { return _a != DBL_MIN ? _a : _a = 4 * (f2_.x_ - f1_.x_) * (f2_.x_ - f1_.x_) - 16 * a_ * a_; }
	double B() { return _b != DBL_MIN ? _b : _b = 8 * (f2_.x_ - f1_.x_) * (f2_.y_ - f1_.y_); }
	double C() { return _c != DBL_MIN ? _c : _c = 4 * (f2_.y_ - f1_.y_) * (f2_.y_ - f1_.y_) - 16 * a_ * a_; }
	double D() { return _d != DBL_MIN ? _d : _d = 4 * T() * (f2_.x_ - f1_.x_) + 32 * a_ * a_ * f2_.x_; }
	double E() { return _e != DBL_MIN ? _e : _e = 4 * T() * (f2_.y_ - f1_.y_) + 32 * a_ * a_ * f2_.y_; }
	double F() { return _f != DBL_MIN ? _f : _f = T() * T() - 16 * a_ * a_ * (f2_.x_ * f2_.x_ + f2_.y_ * f2_.y_); }


	double T() { return f1_.x_ * f1_.x_ + f1_.y_ * f1_.y_ - f2_.x_ * f2_.x_ - f2_.y_ * f2_.y_ - 4 * a_ * a_; }
};

class G {
public:

	// check whether 3 points (x1, y1), (x2, y2), (x3, y3) are in same line
	static bool		is_in_line(double x1, double y1, double x2, double y2, double x3, double y3);
	// check whether p1, p2, p3 are in same line
	static bool		is_in_line(Point p1, Point p2, Point p3) { return is_in_line(p1.x_, p1.y_, p2.x_, p2.y_, p3.x_, p3.y_); }
	static bool		is_in_line(Point*p1, Point*p2, Point*p3) { return is_in_line(*p1, *p2, *p3); }
	static bool		is_in_line(Point*p1, Point*p2, Point p3) { return is_in_line(*p1, *p2,  p3); }

	// check whether x is lies between a, b
	static bool 	is_between(double x, double a, double b);
	// check whether (x, y) is contained by rectangular x1, y1, x2, y2 (except the boundary) or by segment (x1, y1)(x2, y2) (except the vertex)
	static bool 	is_between(double x, double y, double x1, double y1, double x2, double y2);
	// check whether p1 is contained by rectangular with p2 and p3 are non-adjacent vertex
	static bool		is_between(Point p1, Point p2, Point p3) { return is_between(p1.x_, p1.y_, p2.x_, p2.y_, p3.x_, p3.y_); }

	// Check whther (x1, y1)(x2, y2) and (x3, y3)(x4, y4) is intersect
	static bool 	is_intersect(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);

	// side of p to l. 0 : l contain p, <0 side, >0 other side
	static int		position(Point* p, Line* l);
	static int		position(Point  p, Line* l) { return position(&p,  l); }
	static int		position(Point* p, Line  l) { return position( p, &l); }
	static int		position(Point  p, Line  l) { return position(&p, &l); }

	// distance p1 to p2
	static double	distance(Point  p1, Point  p2) { return distance(&p1, &p2); }
	static double	distance(Point* p1, Point  p2) { return distance( p1, &p2); }
	static double	distance(Point  p1, Point* p2) { return distance(&p1,  p2); }
	static double	distance(Point* p1, Point* p2);
	// distance of (x1, y1) and (x2, y2)
	static double	distance(double x1, double y1, double x2, double y2);

	// distance of p and line l
	static double	distance(Point*p, Line*l);
	static double	distance(Point p, Line*l) { return distance(&p,  l); }
	static double	distance(Point*p, Line l) { return distance( p, &l); }
	static double	distance(Point p, Line l) { return distance(&p, &l); }
	// distance of (x, y) and line ax + by + c = 0
	static double 	distance(double x, double y, double a, double b, double c);
	// distance of (x0, y0) and line that contains (x1, y1) and (x2, y2)
	static double	distance(double x0, double y0, double x1, double y1, double x2, double y2);

	// angle of line l with ox
	static Angle	angle(Line* l);
	static Angle	angle(Line  l) { return angle(&l); }

	/**
	 * angle of vector (p1, p2) with Ox - "theta" in polar coordinate
	 * return angle between (-Pi, Pi]
	 */
	static Angle	angle(Point  p1, Point  p2);
	static Angle	angle(Point* p1, Point  p2) { return angle(*p1,  p2); }
	static Angle	angle(Point  p1, Point* p2) { return angle( p1, *p2); }
	static Angle 	angle(Point* p1, Point* p2) { return angle(*p1, *p2); }

	/**
	 * absolute value of angle (p1, p0, p2)
	 */
	static Angle	angle(Point* p0, Point* p1, Point* p2);
	static Angle	angle(Point  p0, Point* p1, Point* p2) { return angle(&p0, p1, p2); }

	/**
	 * angle of vector (p2, p3) to vector (p0, p1)
	 */
	static Angle	angle(Point  p0, Point  p1, Point  p2, Point  p3);
	static Angle	angle(Point* p0, Point* p1, Point* p2, Point* p3) { return angle(*p0, *p1, *p2, *p3); }

	// angle of vector (v2) to vector (v1)
	static Angle	angle(Vector v1, Vector v2);

	// angle from segment ((x2, y2), (x0, y0)) to segment ((x1, y1), (x0, y0))
	static Angle 	angle(double x0, double y0, double x1, double y1, double x2, double y2);

	// Check if segment [p1, p2] is intersect segment [p3, p4]
	static bool		is_intersect(Point* p1, Point* p2, Point* p3, Point* p4);
	static bool		is_intersect(Point* p1, Point* p2, Point* p3, Point  p4) { return is_intersect( p1,  p2,  p3, &p4); }
	static bool		is_intersect(Point* p1, Point* p2, Point  p3, Point  p4) { return is_intersect( p1,  p2, &p3, &p4); }
	static bool		is_intersect(Point  p1, Point  p2, Point  p3, Point  p4) { return is_intersect(&p1, &p2, &p3, &p4); }

	static bool		is_intersect2(Point* p1, Point* p2, Point* p3, Point* p4);
	static bool		is_intersect2(Point* p1, Point* p2, Point  p3, Point  p4) { return is_intersect2(p1, p2, &p3, &p4); }

	static bool		intersection(Point* p1, Point* p2, Point* p3, Point* p4, Point& p);

	// Point that is intersection point of l1 and l2
	static bool		intersection(Line l1, Line l2, Point* p) { return intersection(l1, l2, *p); }
	static bool		intersection(Line l1, Line l2, Point& p);

	// intersection point of the line that contains (x1, y1), (x2, y2) and the line that contains (x3, y3), (x4, y4)
	static bool		intersection(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, double &x, double &y);
	// intersection point of the line a1x + b1y + c1 = 0 and the line a2x + b2y + c2 = 0
	static bool 	intersection(double a1, double b1, double c1, double a2, double b2, double c2, double &x, double &y);

	// Check if ellipse e and line l is intersect
	static int		intersection(Ellipse *e, Line *l, Point &p1, Point &p2);
	static int		intersection(Ellipse  e, Line *l, Point &p1, Point &p2) { return intersection(&e,  l, p1, p2); }
	static int		intersection(Ellipse *e, Line  l, Point &p1, Point &p2) { return intersection( e, &l, p1, p2); }
	static int		intersection(Ellipse  e, Line  l, Point &p1, Point &p2) { return intersection(&e, &l, p1, p2); }

	// midpoint of p1 and p2
	static Point	midpoint(Point *p1, Point *p2) { return midpoint(*p1, *p2); }
	static Point	midpoint(Point  p1, Point *p2) { return midpoint( p1, *p2); }
	static Point	midpoint(Point *p1, Point  p2) { return midpoint(*p1,  p2); }
	static Point	midpoint(Point  p1, Point  p2);

	// vector P1P2
	static Vector	vector(Point *p1, Point *p2) { return vector(*p1, *p2); }
	static Vector	vector(Point *p1, Point  p2) { return vector(*p1,  p2); }
	static Vector	vector(Point  p1, Point *p2) { return vector( p1, *p2); }
	static Vector	vector(Point  p1, Point  p2);

	// vector with slope k
	static Vector	vector(Angle k);

	// line throw p and have slope k
	static Line		line(Point *p, Angle k) { return line(*p, k); }
	static Line		line(Point  p, Angle k);

	// line throw p1 and p2
	static Line 	line(Point *p1, Point* p2) { return line( *p1, *p2); }
	static Line 	line(Point *p1, Point  p2) { return line( *p1,  p2); }
	static Line 	line(Point  p1, Point* p2) { return line(  p1, *p2); }
	static Line 	line(Point  p1, Point  p2);

	// find ax + by + c = 0 that contains (x1, y1) and (x2, y2)
	static void 	line(double x1, double y1, double x2, double y2, double &a, double &b, double &c);

	// line contains p and perpendicular with l
	static Line 	perpendicular_line(Point*p, Line*l);
	static Line 	perpendicular_line(Point p, Line*l) { return perpendicular_line(&p,  l); }
	static Line 	perpendicular_line(Point*p, Line l) { return perpendicular_line( p, &l); }
	static Line 	perpendicular_line(Point p, Line l) { return perpendicular_line(&p, &l); }
	// line contains (x0, y0) and perpendicular with the line that contains (x1, y1) and (x2, y2)
	static void		perpendicular_line(double x0, double y0, double x1, double y1, double x2, double y2, double &a, double &b, double &c);
	// line contains (x0, y0) and perpendicular with line a0x + b0y + c0 = 0
	static void 	perpendicular_line(double x0, double y0, double a0, double b0, double c0, double &a, double &b, double &c);

	// line parallel with l and have distance to l of d
	static void		parallel_line(Line l, double d, Line& l1, Line& l2);
	// line contains p and parallel with l
	static Line		parallel_line(Point* p, Line* l);
	static Line		parallel_line(Point  p, Line* l) { return parallel_line(&p,  l); }
	static Line		parallel_line(Point* p, Line  l) { return parallel_line( p, &l); }
	static Line		parallel_line(Point  p, Line  l) { return parallel_line(&p, &l); }
	// line contains (x0, y0) and parallel with the line that contains (x1, y1) and (x2, y2)
	static void		parallel_line(double x0, double y0, double x1, double y1, double x2, double y2, double &a, double &b, double &c);
	// line that contains (x0, y0) and parallel with line a0x + b0y + c0 = 0
	static void		parallel_line(double x0, double y0, double a0, double b0, double c0, double &a, double &b, double &c);

	// the angle bisector line of (p1 p2 p3)
	static Line		angle_bisector(Point p1, Point p2, Point p3);
	static Line		angle_bisector(Point*p1, Point*p2, Point p3) { return angle_bisector(*p1,*p2, p3); }
	static Line		angle_bisector(Point*p1, Point*p2, Point*p3) { return angle_bisector(*p1,*p2,*p3); }
	// draw the angle bisector ax + by + c = 0 of angle (x1, y1)(x0, y0)(x2, y2)
	static void 	angle_bisector(double x0, double y0, double x1, double y1, double x2, double y2, double &a, double &b, double &c);

	// tangent lines of ellipse e that have tangent point p
	static Line		tangent(Ellipse *e, Point *p);
	static Line		tangent(Ellipse  e, Point *p) { return tangent(&e,  p); }
	static Line		tangent(Ellipse *e, Point  p) { return tangent( e, &p); }
	static Line		tangent(Ellipse  e, Point  p) { return tangent(&e, &p); }

	// tangent lines of circle c that contains p, return tangents line available or not
	static bool		tangent(Circle* c, Point* p, Line& t1, Line& t2);

	// get tangent points of tangent lines of circle that has center O(a, b) and radius r pass through M(x, y)
	static bool		tangent_point(Circle* c, Point* p, Point& t1, Point& t2);
	// get tangent points of tangent lines of circle that has center O(a, b) and radius r pass through M(x, y)
	static bool 	tangent_point(double a, double b, double r, double x, double y, double &t1x, double &t1y, double &t2x, double &t2y);

	// find circumcenter contains p1, p2 and p3
	static Circle	circumcenter(Point p1, Point p2, Point p3) { return circumcenter(&p1, &p2, &p3); }
	static Circle	circumcenter(Point*p1, Point*p2, Point*p3);
	// find circumcenter (xo, yo)
	static void		circumcenter(double x1, double y1, double x2, double y2, double x3, double y3, double &xo, double &yo);

	// quadratic equation. return number of experiment
	static int		quadratic_equation(double a, double b, double c, double &x1, double &x2);

	// area of triangle (ax,ay), (bx,by), (cx,cy)
	static double	area(double ax, double ay, double bx, double by, double cx, double cy);
	// area of triangle p1, p2, p3
	static double	area(Point  p1, Point  p2, Point  p3) { return area(p1.x_, p1.y_, p2.x_, p2.y_, p3.x_, p3.y_); }
	static double	area(Point* p1, Point* p2, Point* p3) { return area(*p1, *p2, *p3); }

	// area of polygon denoted by list of node, head n
	static double	area(node* n);

	// check whether polygon denoted by list of node, head n, is CWW
	static bool		is_clockwise(node*n);

	// find perpendicular bisector of segment ((x1, y1), (x2, y2))
	static void perpendicular_bisector(double x1, double y1, double x2, double y2, double &a, double &b, double &c);


    // extenstion
    static Angle rawAngle(Point* p0, Point* p1, Point* p2, Point* p3);
    static Angle directedAngle(Point *a, Point* p, Point* b);
};

#endif /* GEO_MATH_HELPER_H_ */
