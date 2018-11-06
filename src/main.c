//
// Created by wuyua on 2018-11-05.
//

#include <stdio.h>
#include "VimbaC.h"
#include "camera.h"
int main() {

  VmbVersionInfo_t version;
  VmbVersionQuery(&version, sizeof(version));

  printf("Version = %d.%d.%d \n", version.major, version.minor, version.patch);
  VmbError_t result = VmbStartup();
  printf("result = %d\n", result);

  VmbCameraInfo_t cameraInfo;
  VmbUint32_t numFound;
  VmbCamerasList(&cameraInfo, 1, &numFound, sizeof(cameraInfo));

  printf("Found %d camera(s)\n", numFound);

  if (numFound)
    printf("Camera #1: Name: %s, Id:%s", cameraInfo.cameraName, cameraInfo.cameraIdString);

  getchar();
}