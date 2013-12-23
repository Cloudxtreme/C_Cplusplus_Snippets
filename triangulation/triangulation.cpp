/*************************************************************
 * Triangulation Problem: the basic idea of this implementation
 *   is trying to decompose the polygon to convex 4-sided ones
 *
 * Author: Jinxin Yang
 * PSID: 1168646
 *
 * Input: (x,y) points from file input.data
 *
 * Environment: GNU compiler 4.8.1 are tested
 *   (you could also import this source code into VS project)
 *
 * Compile command:
 *   $ make
 * Execute command:
 *   $ ./triangulation
 *
 * Email me please if you have any questions or problems:
 *   brianyang1106@gmail.com
 *************************************************************
 */

#include <iostream>
#include <fstream>
#include <cmath>

#define M 11

using namespace std;

// sides #
int N;

int Triangulation(int n, double **chord, double **vex, double **weight)
{
  for (int i=1; i<=n; i++) {
    for (int j=1; j<=n; j++) {
      chord[i][j] = 0.0;
    }
  }

  for (int num_4=2; num_4<=n; num_4++) { // for all the 4-sided polygon
    for (int i=1; i<=n-num_4+1; i++) {
      int j = i + num_4 - 1; // tail

      chord[i][j] = chord[i+1][j] + weight[i-1][i] + weight[i][j] + weight[i-1][j];
      // i...j -> i, i+1...j
      vex[i][j] = i;
      // i...j -> i...k, k+1...j
      for (int k=i+1; k<j; k++) {
        if (chord[i][k] + chord[k+1][j] + weight[i-1][k] + weight[k][j] + weight[i-1][j] < chord[i][j]) {
          chord[i][j] = chord[i][k] + chord[k+1][j] + weight[i-1][k] + weight[k][j] + weight[i-1][j];
          vex[i][j] = k;
        }
      }
    }
  }

  return chord[1][N-2];
}

void getTriangle(int i, int j, double **vex) {
  if (i == j) {
    return;
  }

  getTriangle(i, vex[i][j], vex); // Vi, Vj, V(i,j)
  getTriangle(vex[i][j]+1, j, vex); // V(V(i,j)+1,j)
  cout << "triangle vertex: v" << i-1 << ", v" << j << ", v" << vex[i][j] << endl;
}

int main() {
  double x[M], y[M];
  // get the input from input.data
  fstream in("input.data");

  int n = 0;
  while (in) {
    in >> x[n] >> y[n];
    n++;
  } // n-1 hold the number

  N = n;
  n--;

  double **vex = new double *[N];
  double **chord = new double *[N];
  double **weight = new double *[N];
  for(int i=0; i<N; i++) {
    vex[i] = new double[N]; // Vi, Vj, V(i,j) will be a triangle
    chord[i] = new double[N]; // chord record
    weight[i] = new double[N]; // distance
  }

  // calculate the distance of each other
  for (int i=0; i<n; i++) {
    for (int j=0; j<n; j++) {
      weight[i][j] = sqrt((double)(x[i]-x[j])*(x[i]-x[j]) + (y[i]-y[j])*(y[i]-y[j]));;
    }
  }

  double min_chord = Triangulation(N-1, chord, vex, weight);

  cout << "Decompose structure:" << endl;
  getTriangle(1, N-2, vex);

  cout << "\nTotal weight (all the sides of each triangle) = " << min_chord << endl;

  return 0;
}
