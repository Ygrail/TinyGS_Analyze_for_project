#include "Satellites.h"

/*
Output:
0: Raw
1: AX25
*/
int Satellites::coding(int noradid){
    int aux=0;
  switch (noradid)
  {
  case 46276://UPMSAT-2
    aux=1;
    /* code */
    break;
  
  case 51658://INS-2TD
    aux=1;
    /* code */
    break;

  case 43798://ASTROCAST 0.1
    aux=1;
    /* code */
    break;

  default:
    aux=0;
    break;
  }
  return aux;
}
