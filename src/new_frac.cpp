/*
 * Generate space filling curves in an arbitrary region
 *
 * Adrian Bowyer
 * 5 April 2004
 *
 */

#include "new_frac.h"

#define DEBUG 0

static double accy = SMALL;

//***************************************************************

// Geometry functions

// Implicit equation of the line from a to b
//
//    d*x + c = 0

void implicit(const point& a, const point& b, point& d, double& c)
{
  d = b - a;
  double dm = d.mod();
  if((fabs(dm) < accy) && DEBUG)
    cerr << "implicit(...): division by: " << dm << endl;

  d = d/dm;
  c = a.x()*b.y() - b.x()*a.y();
  c = c/dm;
  d = point(-d.y(), d.x());
}

// Distance from a point p to a line segment a-b
 
double line_d(const point& a, const point& b, const point& p)
{
  point ba = b - a;
  point pa = p - a;

  double bam2 = ba.mod2();
  if((bam2 < accy*accy) && DEBUG)
    cerr << "line_d(...): division by: " << bam2 << endl;    
 
  double t = (ba*pa)/bam2;
 
  if(t < 0)
    t = 0;
  if(t > 1)
    t = 1;
 
  ba = a + t*ba - p;
  t = ba.mod();
  return(t);
}

// Calculate the tangents to a circle from a point.  See
// A Programmer's Geometry, p 30.

void tangents(const point& k, const point& j, const double& r, point& tan_d1, 
	      double& tan_c1, point& tan_d2, double& tan_c2)
{
  accy = r*SMALL;

  point kj = k - j;
  double denom = kj.mod2();

  if(denom < accy*accy)
  {
    if(DEBUG) cerr << "tangents(...): centre and point coincide!" << endl;
    return;
  }

  double root = denom - r*r;

  if(root < -accy*accy)
  {
    if(DEBUG) cerr << "tangents(...): point inside the circle!" << endl;
    return;
  }
  
  denom = 1.0/denom;

  if(root < accy*accy)
  {
    tan_d1 = -kj*r*denom;
    tan_d2 = tan_d1;
  } else
  {
    root = sqrt(root);
    tan_d1 = denom*point(-kj.y()*root - r*kj.x(), kj.x()*root - r*kj.y());
    tan_d2 = denom*point(-kj.y()*root + r*kj.x(), kj.x()*root + r*kj.y());
  }
  tan_c1 = -tan_d1*j;
  tan_c2 = -tan_d2*j;
}


//****************************************************************

// Curves class

// Read in all the initial data

curves::curves(const char* file_name)
{
  ifstream ct(file_name);
  if(!ct)
  {
    if(DEBUG) cerr << "curves::curves(...)[1]: Can't open " << file_name << endl;
    return;
  }

  ct >> rad;
  ct >> f;
  ct >> d_max;
  ct >> zag;
  ct >> circles;
  ct >> name;

  char nm[FL_STR];
  ccount = 0;
  while(!ct.eof())
  {
    ct >> nm;
    ccount++;
  }
  ct.close();

  ccount--; // Counts the last line twice...

  c = new LinkSeg*[ccount + 3];
  ends = new LinkSeg*[ccount + 3];

  ifstream ip(file_name);
  if(!ip)
  {
    if(DEBUG) cerr << "curves::curves(...)[2]: Can't open " << file_name << endl;
    return;
  }

  ip >> rad;
  ip >> f;
  ip >> d_max;
  ip >> zag;
  ip >> circles;
  ip >> name;

  for(int i = 0; i < ccount; i++)
  {
    ip >> nm;
    c[i] = read(nm, &ends[i]);
  }

  c[ccount] = 0;
  ends[ccount] = 0;

  ip.close();
}

// loop-split segments in c (c[0] is unsplit as it is the boundary).

void curves::r_split()
{
  LinkSeg *here;
  LinkSeg *ht;
  int i;

  int depth = 1;

  while(depth <= d_max)
  {
    i = 1;
    while(c[i])
    {
      here = c[i];
      do
      {
	ht = here->after();
	split(here, c, rad);
	here = ht;
      } while(here && (here != c[i]));
      rad = rad*f;
      i++;
    }
    depth++;

    if(zag && (depth <= d_max))
    {
      i = 1;
      while(ends[i])
      {
        here = ends[i];
	do
        {
	  ht = here->before();
	  split(here, c, rad);
	  here = ht;
	} while(here && (here != ends[i]));
	rad = rad*f;
        i++;
      }
      depth++;
    } 
  }
  rad = rad/f;
}

void curves::picture()
{
  LinkSeg* here = c[0];
  double xmin = here->pt().x();
  double ymin = here->pt().y();
  double xmax = xmin;
  double ymax = ymin;
  double x, y;
  do
  {
    x = here->pt().x();
    y = here->pt().y();
    if(x > xmax) xmax = x;
    if(x < xmin) xmin = x;
    if(y > ymax) ymax = y;
    if(y < ymin) ymin = y;
    here = here->after();
  } while(here && (here != c[0]));

  x = (xmax - xmin)*0.05;
  y = (ymax - ymin)*0.05;

  frame(xmin - x, ymin - y, xmax + x, ymax + y);

  int i = 0;
  double r;
  if(!circles)
    r = -1;
  while(c[i])
  {
    r = rad;
    if(!circles || !i)
      r = -1;
     c[i++]->picture(0, r, !i);
  }

  draw();
  save(name);
}


void curves::statistics(ostream *ofs, int head)
{
  int i = 1;

  if(head)
    *ofs << "count" << ',' << "sum" << ',' << "sum^2" << 
      ',' << "max" << ',' << "min" << '\n';

  while(c[i])
  {
     c[i++]->statistics(ofs);
  }
}

//*****************************************************************

// LinkSeg class

// Split the segment from aa, not coming closer than rad to anything
// in c0 or c1.  Return TRUE if successful, FALSE if not

void split(LinkSeg* aa, LinkSeg** c_0, double rad)
{
  if(!aa->after()) return;

  point a = aa->pt();
  point b = aa->after()->pt();

  point d = b - a;

  // Closer than 2*rad apart?  If so, don't split

  //if(d.mod2() <= 4*rad*rad) return;

  l_break lb(aa, rad);
 
  LinkSeg* here;
  int i = 0;
  while(c_0[i])
  {
    here = c_0[i];
    do
    {
      if(here != aa)
      {
	if(here != aa->after())
	  lb.tangent(here->pt());
	if(here->after())
	  lb.distance(here->pt(), here->after()->pt());
      }
      here = here->after();
    } while(here && here != c_0[i]);
    i++;
  }

  aa->after(lb.pt());

}

// Plot a picture of a chain of segments

void LinkSeg::picture(int solid, double r, int loop)
{
  LinkSeg* here;

  here = this;
  colour(r, g, b);
  move(here->pt().x(), here->pt().y());
  if(r > 0) mark(here->pt().x(), here->pt().y(), r, solid);
  here = here->after();
  do
  {
    colour(here->r, here->g, here->b);
    plot(here->pt().x(), here->pt().y());
    if(r > 0)mark(here->pt().x(), here->pt().y(), r, solid);
    here = here->after();
  } while(here && (here != this));
  if(loop)
    plot(pt().x(), pt().y());
}

// Print statistics to a stream

void LinkSeg::statistics(ostream *ofs)
{
  LinkSeg* here;
  double sum = 0;
  double sum2 = 0;
  double min = BIG;
  double max = 0;
  double x, xd, y, yd, s;
  int count = 0;

  here = this;
  x = here->pt().x();
  y = here->pt().y();
  here = here->after();
  do
  {
    xd = x - here->pt().x();
    yd = y - here->pt().y();
    s = xd*xd + yd*yd;
    sum2 += s;
    s = sqrt(s);
    if(s > max) max = s;
    if(s < min) min = s;
    sum += s;
    count++;
    x = here->pt().x();
    y = here->pt().y();
    here = here->after();
  } while(here && (here != this));

  *ofs << count << ',' << sum << ',' << sum2 << ',' << max << ',' << min << '\n';
}



// Read in a chain from a file:
//
// 1/0   (loop or not)
// r g b (colour)
// x0 y0
// x1 y1
// ...
// xn yn
//
// If loop is TRUE, the last point is linked back to the first to form a loop

LinkSeg* read(const char* file_name, LinkSeg** end)
{
  ifstream ip(file_name);
  if(!ip)
  {
    if(DEBUG) cerr << "read(...): Can't open " << file_name << endl;
    return 0;
  }

  double x, y, r, g, b;
  point p;
  int loop;

  ip >> loop;
  ip >> r >> g >> b;
  ip >> x >> y;
  p = point(x, y);

  LinkSeg* l = new LinkSeg(p, r, g, b);
  LinkSeg* l_first = l;

  while(!ip.eof())
  {
    ip >> x >> y;
    if(!ip.eof())
    {
      p = point(x, y);
      l->after(p, r, g, b);
      l = l->after();
    }
  }

  ip.close();

  *end = l;

  if(loop) 
  {
    l->n = l_first;
    l_first->p = l;
  }

  return l_first;
}


// Write the chain FROM LINK l to a file

void write(const char* file_name, LinkSeg* l)
{
  ofstream op(file_name);
  if(!op)
  {
    if(DEBUG) cerr << "Can't open " << file_name << endl;
    return;
  }

  LinkSeg* first = l;

  do
  {
    op << l->pt().x() << '\t' << l->pt().y() << '\n';
    l = l->after();
  }while(l && (l != first));

  op.close();
}

//***********************************************************

// l_break class

static double cnt = 0;

void l_break::tan_update(const point& tan_d, const double& tan_c, int side)
{
  double t;
  double denom = tan_d*d0;
  
  if(fabs(denom) > accy)
  {
    t = -(tan_c + tan_d*mid)/denom;

    // Update the max and min t values

    if(side)  // p on t -ve side
    {
      if(t < accy)
      {
	if(t > t_min) t_min = t;
	if(t_min > 0)
	  t_min = 0;
      } 
    } else              // p on t +ve side
    {
      if(t > -accy)
      {
	if(t < t_max) t_max = t;
	if(t_max < 0)
	  t_max = 0;
      } 
    }
  }
}

// Tangent clasification

void l_break::tangent(const point& p)
{
  // The tangent from a (or b) to the disc r about p
  // is tan_d*x + tan_c = 0

  accy = r*SMALL;
  point tan_d1, tan_d2;
  double tan_c1, tan_c2, t, denom;
  point n = a;
  point q;

  int side = 0;
  if(d0*p + c0 < 0)
    side = 1;

  for(int j = 1; j > -2; j = j - 2)
  {
    t = (d1*p + c1)*j;
    if((t > 0) && (t < half+r)) // On the same side of the bisector as n?
    {
      tangents(p, n, r, tan_d1, tan_c1, tan_d2, tan_c2);

      // Now the tangents are known, where do they cut the bisector?

      tan_update(tan_d1, tan_c1, side);
      tan_update(tan_d2, tan_c2, side);
    }
    n = b;
  }
}
 
void l_break::distance(const point& p, const point& q)
{
  point d = q - p;

  accy = r*SMALL;

  double denom = d*d1;
  if(fabs(denom) < accy)
    return;

  double t = -(c1 + d1*p)/denom;

  if (t < 0) return;
  if (t > 1) return;

  point dpq;
  double cpq;

  implicit(p, q, dpq, cpq);

  denom = dpq*d0;
  if(fabs(denom) < accy)
    return;

  t = -(cpq + dpq*mid)/denom;

  double dm = d.mod();
  if((dm < accy) && DEBUG)
    cerr << "l_break::distance(...)[1]: division by " << dm << endl;

  d = d/dm;
  denom = d*d0;
  denom = sqrt(1 - denom*denom);
  if((denom < accy) && DEBUG)
    cerr << "l_break::distance(...)[2]: division by " << denom << endl;
  denom = 1.0/denom;

  if(t < 0)
  {
    t = t + r*denom;
    if(t > t_min)
      t_min = t;
    if(t_min > 0)
      t_min = 0;
  } else
  {
    t = t - r*denom;
    if(t < t_max)
      t_max = t;
    if(t_max < 0)
      t_max = 0;
  }
}

//***********************************************************




