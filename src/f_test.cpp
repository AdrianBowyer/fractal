#include "new_frac.h"

int main(int argc, char**argv)
{
  if(argc != 2)
  {
    cerr << "usage: forte control_file" << endl;
    return 1;
  }

  //  for(int i = 0; i < 16; i++)
  // {

    curves c(argv[1]);

    // c.set_dm(i);

    c.r_split();

    c.picture();

    // c.statistics(&cout, !(i));
    // }

  return 0;
}
