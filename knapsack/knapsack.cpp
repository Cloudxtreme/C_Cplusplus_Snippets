/*************************************************************
 * knapsack problem: Recursive and Non-recursive (with stack)
 * Class version:
 *   Given a target t, and a collection of positive integer
 *   weights w1, w2, ..., wn, determine whether there is
 *   some selection from the weights that totals exactly t.
 *
 * Author: Jinxin Yang
 * PSID: 1168646
 *
 * Implementation: two separate functions for two versions
 *   - Recursive:      knapsack()
 *   - Non-recursive:  knapsack_stack()
 *
 * Input: Default target size t and weights[]
 *
 * Environment: GNU compiler 4.1.2 / 4.7.2 / 4.8.1 are tested
 *   (you could also import this source code into VS project)
 *
 * Compile command:
 *   $ make
 * Execute command:
 *   $ ./knapsack
 *
 * Email me please if you have any questions or problems:
 *   brianyang1106@gmail.com
 *************************************************************
 */

#include <iostream>
#include <stack>

using namespace std;

/* maximum number of packages */
#define MAX 10

/* default input */
int t = 27;
int weights[MAX] = {3, 2, 1, 6, 5, 4, 9, 8, 7, 10};
stack <int> s;

/* knapsack recursive version*/
bool knapsack (int target, int cand) {
  if (target == 0) {
    return true;
  }
  else if (target<0 || cand > MAX) {
    return false;
  }
  else if (knapsack(target - weights[cand], cand+1)) {
    cout << weights[cand] << " ";
    return true;
  }
  else {
    return (knapsack(target, cand + 1));
  }
}

/* knapsack non-recursive version
 */
bool knapsack_stack (int target, int cand) {
  int none;

  for(cand=0; cand<=MAX; cand=none) {
    none = 0;

    while (cand < MAX) {
      target = target - weights[cand];

      if (target >= 0) {
	s.push(weights[cand]);
	none = cand;

        // package's ready
	if (target == 0) {
	  return true;
	}
      } else { // put the package in
	target = target + weights[cand];
      }
      cand++;
    }

    // pop all the previous packages
    if (!s.empty()) {
      s.pop();
      target = target + weights[none];
      none--;
    }
    none = none + 2;
  }

  return false;
}

void print_info () {
  cout << "  Target: " << t << endl;
  cout << " Weights: ";
  int i;
  for (i=0; i<MAX; i++) {
    cout << weights[i] << " ";
  }
  cout << endl;
}

int main () {
  int cand = 0;
  print_info();

  // recursive
  cout << "\nRecursive Solution: ";
  if (!knapsack(t, cand)) {
    cout << "Not found..." << endl;
  }
  cout << endl;

  // stack
  cout << "\nNon-recursive Solution: ";
  if (knapsack_stack(t,cand)) {
    while (!s.empty()) {
      cout << s.top() << " ";
      s.pop();
    }
  }
  else {
    cout << "Not found..." << endl;
  }
  cout << endl;

  return 0;
}
