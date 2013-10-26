#include <iostream>
#include <cstdlib>
#include <string.h>
#include <algorithm>
#include <fstream>
#include <vector>

using namespace std;

int out_depth;

int 
getDepth (char *neworder) {
  int out_depth=0;
  for (int i=0; i<strlen(neworder); i++) {
    if (neworder[i] == ',') {
      out_depth++;
    }
  }
  return out_depth;
}

void 
getName (int depth, char *neworder, char *name) {
  int comma=0;

  int i=0;
  while (comma!=depth) {
    if (neworder[i] == ',') {
      comma++;
    }
    i++;
  }
  
  int j=i;
  i=0;
  while (neworder[j]!=',' && neworder[j]!='\0') {
    name[i] = neworder[j];
    i++;
    j++;
  }
  name[i] = '\0';
}

void 
toupper (char *name) {
  for (int i=0; i<strlen(name); i++) {
    if (name[i] >= 'a' && name[i] <= 'z') {
      name[i] = name[i] - 32;
    }
  }
}

void 
itoa (int num, char * ch) {
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

// delete file
bool
rm (char filename[]) {
  ifstream tmp;
  tmp.open(filename);
  
  if (tmp.fail()) { // file doesn't exist!
    tmp.close();
    return false;
  } else {
    char deleteFile[30] = "/bin/rm ";
    strcat(deleteFile, filename);
    system(deleteFile);
    tmp.close();
    return true;
  }
}

/*
 * compare function tests all 
 * the results of matrix_C output
 * after corresponding interchange
 * compare matrix_ori to matrix_i
 * i = 0, 1, 2, ...
 */
void
compare (int out_depth, int count, char infilename[]) {
  char buffer[4];
  itoa(out_depth+1, buffer);
  char ori_file_name[20] = "matrix_";
  strcat(ori_file_name, buffer);
  char prefix[20];
  strcpy(prefix, ori_file_name);
  strcat(ori_file_name, infilename); // matrix_4_per_v1/2.f

  ofstream com_result;
  com_result.open("com_result");

  for (int i=0; i<count; i++) {
    // open matrix_n_ori
    ifstream ori_file;
    ori_file.open(ori_file_name);
    bool open_fail = ori_file.fail();

    char com_file_name[20];
    strcpy(com_file_name, prefix); // prefix = matrix_n
    strcat(com_file_name, "_");
    
    char id_buffer[4];
    itoa(i, id_buffer);
    strcat(com_file_name, id_buffer);

    ifstream com_file;
    com_file.open(com_file_name);
    //cout<<endl<<ori_file_name<<"  "<<com_file_name<<endl;

    double num_ori, num_com;
    com_result<<i<<","; // combination_id
    
    bool flag = true;
    while (ori_file>>num_ori && com_file>>num_com) {
      //cout<<endl<<num_ori<<"  "<<num_com<<endl;
      if (num_ori != num_com) {
	flag = false;
	break;
      }
    }

    if (flag && !open_fail) {
      com_result<<"1"<<endl;
    } else {
      com_result<<"0"<<endl;
    }

    // if you don't want to delete the temporary result
    // you could comment the following sentence
    rm(com_file_name);

    ori_file.close();
    com_file.close();
  }
  
  com_result.close();
}

void
generateXML (int out_depth, int line_num, char neworder[], 
	     char directive[], std::vector<int> loops) {
  // just permutation for now
  char name[10];
  ofstream xml;
  xml.open("permutation.xml");
  
  xml<<"<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n";
  xml<<"<transformation>\n"; // indent size: 2 spaces
  
  do {
    xml<<"  <permutation>\n";
    xml<<"    <line_num>"<<line_num<<"</line_num>\n";
    xml<<"    <directive>"<<directive<<"</directive>\n";

    xml<<"    <order>(";
    for(int i = 0; i <=out_depth; i++) {
      if (i != 0) {
	xml<<",";
      }
      getName(loops[i], neworder, name);
      xml<<name;
    }

    xml<<")</order>\n";
    xml<<"  </permutation>\n\n";
  }while (next_permutation(loops.begin(), loops.end()));

  xml<<"</transformation>\n";
  xml.close();
}

/////////////////////
// main
int
main (int argc, char **argv) {
  // neworder format: i,j,k,l... (no space)
  char *infilename=NULL, *neworder=NULL, *st;
  bool step;

  if (argc < 3 ) {
    printf("usage: ./a.out <infilename> <i,j,k...> [-s(step)]\n");
    return 0;
  }

  infilename = strdup (argv[1]);
  neworder = strdup (argv[2]);
  out_depth = getDepth(neworder);
  if (argc == 4) {
    st = strdup (argv[3]);
    if (!strcmp(st, "-s")) {
      step = true;
    } else {
      printf("usage: ./a.out <infilename> <i,j,k...> [-s(step)]\n");   
      return 0;
    }
  }

  char name[20];
  
  std::vector<int> loops;
  for (int i=0; i<=out_depth; i++) {
    loops.push_back(i);
  }
  sort(loops.begin(), loops.end());

  // generating the XML
  generateXML(out_depth, 22, neworder, "C*$* interchange", loops);

  rm("pragma");
  rm("result");

  int count = 0;
  do {
    ifstream in;
    ofstream out, cnt;
    in.open(infilename);
    char outfilename[20] = "__";
    strcat(outfilename, infilename);
    out.open(outfilename);

    // let the backend know which round is running
    cnt.open("count");
    cnt<<count;
    cnt.close();

    char c;
    in.get(c);
    while (in) {
      // 1. for interchange
      if (c == '[') {
	in.get(c);
	if (c == ']') {
	  //out<<neworder;
	  ofstream orig;
	  orig.open("pragma", ios::app);
	  cout<<"\nPragma: ";
	  orig<<count<<','; // tell backend

	  for(int i = 0; i <=out_depth; i++) {     
	    if (i != 0) {
	      out<<",";
	      cout<<",";
	      orig<<",";
	    }
	    getName(loops[i], neworder, name);
	    out<<name;
	    toupper(name);
	    cout<<name;
	    orig<<name;
	  }
	  orig<<endl;
	  orig.close();
	}
      } else if (c == '@') {
	out<<count; // for write the matrix_C
      } else {
	out<<c;
      }

      in.get(c);
    }

    in.close();
    out.close();

    out.open("test.sh");
    out<<"cd ./\n\nuhf90 -O3 ";
    if (step) {
      out<<"-flist -show -Wb,-ttLNO:0x0004 ";
    }
    out<<outfilename<<endl;
    out<<"./a.out"<<endl;
    // -LNO:minvar=off:interchange=off:fission=0:ou_max=1:blocking=off:oinvar=off 
    out.close();

    system("/bin/sh test.sh");

    if (step) {
      cout<<"\n\nENTER to continue...\n";
      char ch;
      cin.get(ch);
    }
    
    count++;
    cout<<endl<<endl;
  } while (next_permutation(loops.begin(), loops.end()));

  cout<<endl;

  // compare
  compare(out_depth, count, infilename);
  system("/bin/sh dump.sh");
  return 0;
}
