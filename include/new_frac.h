/*
 * Generate space filling curves in an arbitrary region
 *
 * Adrian Bowyer
 * 5 April 2004
 *
 */

#ifndef space_fill_h
#define space_fill_h

#include <graphics.h>
#include "unistd.h"  
#include <iostream>
#include <fstream>
#include <math.h>

#define FL_STR 1024  // Max chars in file names

// Fudge factors etc...

#define CLOSE 0.025
#define SMALL 0.00000001
#define BIG 1.0e100


using namespace std;
 
// Sign function (NB 0 => -1)

inline double sign(double a)
{
        return( (a > 0) ? 1.0 : -1.0 );
}

//***************************************************************

// Vectors

class point
{
 private:

  double xx, yy;

 public:

  point(double a = 0, double b = 0) { xx = a; yy = b; }

  // Get the numbers back out

  double x() const { return xx; }
  double y() const { return yy; }
  double mod2() const { return xx*xx + yy*yy; }
  double mod() const { return sqrt(mod2()); }

  void print() const { cout << "(" << xx << ", " << yy << ")"; }
  void printt(char* t) const { cout << t << ": "; print(); }
  void printl() const { print(); cout << endl; }

  // Vector arithmetic

  friend point operator+(const point &a, const point &b);
  friend point operator-(const point &a);
  friend point operator-(const point &a, const point &b);
  friend double operator*(const point &a, const point &b);
  friend point operator*(const point &a, double b);
  friend point operator*(double b, const point &a);
  friend point operator/(const point &a, double b);
};

// Vector addition

inline point operator+(const point &a, const point &b)
{
    point r;
    r.xx = a.xx + b.xx;
    r.yy = a.yy + b.yy;
    return r;
}

// Vector negation

inline point operator-(const point &a)
{
    point r;
    r.xx = -a.xx;
    r.yy = -a.yy;
    return r;
}

// Vector subtraction

inline point operator-(const point &a, const point &b)
{
    return a + (-b);
}

// Scalar product

inline double operator*(const point &a, const point &b)
{
    return a.xx*b.xx + a.yy*b.yy;
}

// Scale

inline point operator*(const point &a, double b)
{
    return point(a.xx*b, a.yy*b);
}
inline point operator*(double b, const point &a)
{
    return point(a.xx*b, a.yy*b);
}
inline point operator/(const point &a, double b)
{
    return point(a.xx/b, a.yy/b);
}

// Implicit equation of the line from a to b
//
//    d*x + c = 0

void implicit(const point& a, const point& b, point& d, double& c);

//**************************************************************

// Linked list of points forming a chain

class LinkSeg
{
 private:
  point pnt;       // Coordinates of the point
  LinkSeg* n;      // Next one
  LinkSeg* p;      // Previous one
  double r;        // Red
  double g;        // Green
  double b;        // Blue

 public:

  // Start one with a single point and a colour

  LinkSeg(const point& pt, double rr, double gg, double bb)
  {
    pnt = pt;
    n = 0;
    p = 0;
    r = rr;
    g = gg;
    b = bb;
  }

  // Default colour is white

  LinkSeg(const point& pt)
  {
    pnt = pt;
    n = 0;
    p = 0;
    r = 1;
    g = 1;
    b = 1;
  }

  // Add a point before

  void before(LinkSeg* bf)
  {
    bf->p = p;
    if(p) p->n = bf;
    bf->n = this;
    p = bf;
  }

  void before(const point& pt)
  {
    LinkSeg* bf = new LinkSeg(pt, r, g, b);
    before(bf);
  }

  void before(const point& pt, double rr, double gg, double bb)
  {
    LinkSeg* bf = new LinkSeg(pt, rr, gg, bb);
    before(bf);
  }

  // Get the point before

  LinkSeg* before() const { return p; }

  // Add a point after

  void after(LinkSeg* af)
  {
    af->n = n;
    if(n) n->p = af;
    af->p = this;
    n = af;
  }

  void after(const point& pt)
  {
    LinkSeg* af = new LinkSeg(pt, r, g, b);
    after(af);
  }

  void after(const point& pt, double rr, double gg, double bb)
  {
    LinkSeg* af = new LinkSeg(pt, rr, gg, bb);
    after(af);
  }

  // Get the point after

  LinkSeg* after() const { return n; }

  // Get the vector

  point pt() const { return pnt; }

  // Plot it

  void picture(int er, double r, int loop);

  // Output stats

  void statistics(ostream *ofs);

  friend LinkSeg* read(const char* file_name, LinkSeg** end);
};

void split(LinkSeg* aa, LinkSeg** c_0, double rad);

//*********************************************************

// Class to find the best point for a break along the perpendicular
// bisector of a-b.

class l_break
{
 private:

  point a;               // Point at one end of segment
  point b;               // Point at the other
  point d0;              // Implicit equation is ...
  double c0;             // ... d0*x + c0 = 0
  point d1;              // Implicit equation of bisector is ...
  double c1;             // ... d1*x + c1 = 0, and the ...
  point mid;             // parametric version is p = mid + d0*t
  double t_max, t_min;   // Best positive and best negative t values
  double r;              // Cordon sanitaire
  double half;           // Half the line length
  LinkSeg* avoid;        // Link not to test against

 public:

  l_break(LinkSeg* lk, double rad)
  {
    a = lk->pt();
    if(!lk->after())
      cerr << "l_break(...): no subsequent point." << endl;
    b = lk->after()->pt();
    avoid = lk;

    r = rad;
    mid = (a + b)*0.5;
    implicit(a, b, d0, c0);
    implicit(mid, mid+d0, d1, c1);
    half = 0.5*((b - a).mod());
    t_max = BIG;
    t_min = -BIG;
  }

  void tangent(const point& p);
  void tan_update(const point& tan_d, const double& tan_c, int side);
  void distance(const point& p, const point& q);

  // Return the point

  point pt() const
  {
    double t = t_max;
    if(fabs(t_min) > fabs(t_max))
      t = t_min;
    point p = mid + d0*t;
    return p;
  }

  //int scan(LinkSeg *c_0, const point& p);
  //void test(LinkSeg* c_0, LinkSeg* c_1);
};

class curves
{
 private:
  LinkSeg** c;       // The curves in an array
  LinkSeg** ends;    // The ends of the curves
  int ccount;        // The number of curves
  double rad;        // The initial radius
  double f;          // The reduction factor
  int d_max;         // The recursion depth
  int zag;           // Recursion pattern
  int circles;       // Plot the circles?
  char name[FL_STR]; //Graphics file for the results

 public:

  // Read in the initial data

  curves(const char* filename);

  // Recursively split the set of segments (0th is assumed to
  // be the boundary)

  void r_split();

  void set_dm(int d) { d_max = d; }

  // Plot a picture

  void picture();

  // Print stats

  void statistics(ostream *ofs, int head);
};

void write(const char* file_name, LinkSeg* l);

#endif
