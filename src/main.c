//
// Created by wuyua on 2018-11-05.
//

#include <stdio.h>
#include "VimbaC.h"
#include "camera.h"
int main() {

  VmbVersionInfo_t version;
  VmbVersionQuery (&version, sizeof(version));

  printf ("Version = %d.%d.%d \n", version.major, version.minor, version.patch);
  test ();

  getchar ();
}