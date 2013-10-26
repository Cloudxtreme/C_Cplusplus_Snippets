#include <iostream>
#include <cstdlib>
#include <string.h>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <fstream>
#include <vector>

#define DIR_INTERCHANGE "C*$* INTERCHANGE"
#define DIR_UNROLL "C*$* UNROLL"
#define DIR_BLOCKING_SIZE "C*$* BLOCKING SIZE"

using namespace std;

/*
 * show the instruction of running program
 */
void instr () {
  printf("Usage: ./a.out filename filename | -keep\n");
  printf("\n-keep will keep all the combination files\n");
  exit(0);
}

/*
 * bug of linux ctype.h
 */
int ToUpper(int c)
{
  return toupper(c);
}

bool hasKEY (string str, string key) {
  // toUpper
  transform(str.begin(), str.end(), str.begin(), ToUpper);

  if (str.find(key) == 0) {
    // has to start from 0 (or maybe !)
    return true;
  }

  return false;
}

/*
 * format output vector elements with delim
 */

void printV (ostream &out, vector<string> vec, string delim) {
  for (vector<string>::iterator iter = vec.begin();
       iter!=vec.end(); iter++) {
    if (iter != vec.begin()) {
      out << delim;
    }

    out << *iter;
  }
}

void printV (ostream &out, vector<int> vec, string delim) {
  for (vector<int>::iterator iter = vec.begin();
       iter!=vec.end(); iter++) {
    if (iter != vec.begin()) {
      out << delim;
    }

    out << *iter;
  }
}

/*
 * split the string with delimiter
 * TODO: trim the prefix and suffix spaces!
 */
vector<string> split (string str, string delim) {
  vector<string> strvec;
  
  string::size_type pos1, pos2;
  pos2 = str.find(delim);
  pos1 = 0;       
  while (string::npos != pos2) {
    strvec.push_back(str.substr(pos1, pos2 - pos1));
               
    pos1 = pos2 + 1;
    pos2 = str.find(delim, pos1);
  }
  strvec.push_back(str.substr(pos1));

  return strvec;
}


/*
 * only for interchange order extraction
 */
vector<string> getOrderMap (string line) {
  int left = line.find("(");
  int right = line.find(")");
  string entire_order = line.substr(left+1, right-left-1);
  vector<string> ori_order = split(entire_order, ",");

  return ori_order;
}

/*
 * return new INTERCHANGE order
 */
string getNewOrder (vector<string> ori, vector<int> loops) {
  string str;
  
  int i;
  for (i=0; i<loops.size(); i++) {
    if (i!=0) {
      str.append(",");
    }
    str.append(ori[loops[i]]);
  }

  return str;
}

void itoa (int num, char * ch) {
  int i=0, j;
  char tmp;
  if (num == 0) {
    ch[0] = '0';
    ch[1] = '\0';
    return;
  }
  while (num != 0) {
    ch[i] = '0' + num%10;
    i++;
    num=num/10;
  }
  ch[i]='\0';
  for (j=0; j<i/2; j++) {
    tmp = ch[j];
    ch[j]=ch[i-1-j];
    ch[i-1-j]=tmp;
  }
}

string getSizeDIR (string dir, int size) {
  dir.append(" (");
  char tmp[20];
  itoa(size, tmp);
  dir.append(tmp);
  dir.append(")");

  return dir;
}

void compile (int num, char *FILENAME) {
  char command[64] = "uhf90 -O3 -o ./uhf90/";
  char comma[4] = ".";
  char space[4] = " ";
  strcat(command, FILENAME);
  strcat(command, comma);
  char tmp_num[8];
  itoa(num, tmp_num);
  strcat(command, tmp_num);
  strcat(command, space);
  strcat(command, FILENAME);

  system(command);
}

//////////////////////////////////////////////////
int main (int argc, char **argv) {
  
  if (argc != 2 && argc != 3) {
    instr();
  }

  char *infilename = NULL; // input file
  char *FILENAME = NULL;
  infilename = strdup (argv[1]);
  
  bool keep = false;
  if (argc == 3) {
    char *hasKEEP = NULL;
    hasKEEP = strdup (argv[2]);
    if (! strcmp(hasKEEP, "-keep")) {
      keep = true;
    } else {
      FILENAME = strdup (argv[2]);
    }
  }
  
  ifstream in(infilename);
  if (!in) {
    cerr << "Input file open error!\n";
    return -1;
  }

  // process the entire input file
  string line;

  while (getline(in, line)) {
    if (hasKEY(line, DIR_INTERCHANGE)) {
      /*
       * here we could use interchange,
       * and unroll and blocking size 
       * should be following and processed
       */
      vector<string> ori_order = getOrderMap(line);
      vector<int> loops; // for STL next_permutation
      for (int i=0; i<ori_order.size(); i++) {
	loops.push_back(i);
      }

      /*
       * simple directive order checking
       * should be interchange, unroll, blocking size
       */
      getline(in, line);
      if (hasKEY(line, DIR_UNROLL)) {
	getline(in, line);
	if (! hasKEY(line, DIR_BLOCKING_SIZE)) {
	  cerr << "BLOCKING SIZE should follow UNROLL" << endl;
	  return -1;
	}
      } else {
	cerr << "UNROLL should follow INTERCHANGE" << endl;
	return -1;
      }

      in.close();
      /*
       * start to traverse all the interchange order
       */
      int global_counter = 0;
      int max_unroll = 8, max_blocking_size = 6;
      ofstream out, TFILE;
      TFILE.open("temp.t");

      do {
	string tmp_neworder = getNewOrder(ori_order, loops);

	int i, j; // i -> unroll, j -> blocking

	for (i=1; i<=max_unroll; i++) {
	  for (j=1; j<=max_blocking_size; j++) {
	    in.open(infilename);
	    out.open(FILENAME);

	    while (getline(in, line)) {
	      if (hasKEY(line, DIR_INTERCHANGE)) {
		getline(in, line); // skip unroll
		getline(in, line); // skip blocking size
		break;
	      } else {
		out << line << endl;
	      }
	    }

	    TFILE << global_counter << ".   "
		  << tmp_neworder << "   ";
	    TFILE << "unroll = " << i << "   ";
	    TFILE << "blocking = " << j << "   \n";

	    // replace original dirs
	    out << DIR_INTERCHANGE << " ("
		<< tmp_neworder
		<< ")" << endl;
	    out << getSizeDIR(DIR_UNROLL, i) << endl;
	    out << getSizeDIR(DIR_BLOCKING_SIZE, j) << endl;

	    while (getline(in, line)) {
	      out << line << endl;
	    }

	    compile(global_counter, FILENAME);
	    global_counter++;

	    in.close();
	    out.close();
	  }
	}

      } while (next_permutation(loops.begin(), loops.end()));

      TFILE.close();
    } // if
    /*else { // output to temp files
      out << line << endl;
      }*/
  } // while

  return 0;
}
