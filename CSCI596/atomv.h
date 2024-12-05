#pragma once
/***********************************************************************
  Enhanced atomv.h with support for stress tensor and trajectory tracking
***********************************************************************/
#include <stdbool.h>

// Structure to store trajectory points
typedef struct {
    float x, y, z;
} TrajectoryPoint;

typedef struct {
    float crd[3];
    float stress;  // Stress tensor (simplified to scalar for demonstration)
} AtomType;

int nlon = 18, nlat = 9;
float atom_radius = 0.2;
int winx = 640, winy = 640;
bool tracking_enabled = false;
float min_ext[3], max_ext[3];
int natoms;
AtomType* atoms;
float eye[3];
float center[3];
float up[3];

