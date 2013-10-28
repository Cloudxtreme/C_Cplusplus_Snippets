#include "rose.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <string.h>
#include <cstdlib>
#include <fstream>
#include <algorithm>

#define TAC_NUM 9 // TAC attributes number
#define ST_NUM 3 // symbol table attributes number

using namespace std;
using namespace SageInterface;
using namespace SageBuilder;

//////  my own works //////////////////////////////
void itoa (int num, char * ch);

/// global varibles and functions for reg_alloc ///
int assign_i=2; // to store the number of assginment statements
int Reg_tx[33][3]; // 32 register at most, [][2] is visited or not
// Reg_tx[i][] means Ri, Reg_tx[][1] stores the times of used
// Reg_tx[0][0] stores the number of registers has been colored

void resetRegTx() {
  int i;
  for (i=0; i<33; i++) {
    Reg_tx[i][0]=0; // tx label
    Reg_tx[i][1]=0; // times of Ri used until now
    Reg_tx[i][2]=0; // still remain in the reg or not in next stmt
  }
}

void resetRegTx(int which) { // careful use!
  int i;
  for (i=0; i<33; i++) {
    Reg_tx[i][which]=0; // still remain in the reg or not in next stmt
  }
}

bool checkFull(int num) { // check whether registers are full
  int i;
  bool flag = true;
  for (i=1; i<=num; i++) {
    if (Reg_tx[i][2]!=1) {
      flag = false;
      break;
    }
  }
  return flag;
}

void reg_alloc(std::vector<int> outVars, int num);

/////////////all my classes here///////////////////
// symbol table
typedef struct SNode
{
  char name[20];
  char type[20];
  char value[20];
  struct SNode *next;
} *symbolList;

class symTable
{
private:
  symbolList stable;
public:
  int tx; // temporary variables tracker
  int ftx; // float temporary variables tracker

public:
  symTable () {
    symbolList L=NULL;
    L = (symbolList)malloc(sizeof(SNode));
    L->next=NULL;
    stable = L;
    tx=2;
    ftx=1;
  }

  symTable (string a[][3], int n) {
    createList (stable, a, n);
    tx=2; // before loop, we assume t1 <- N
    ftx=1; // start from 1
  }

  void updateTX() {
    tx++;
  }
  void updateFTX() {
    ftx++;
  }

  void createList (symbolList &L, string a[][3], int n) {
    int i;
    symbolList s=NULL, r=NULL; // temp
    L = (symbolList)malloc(sizeof(SNode));

    L->next = NULL; // header node
    r = L;

    for (i=0; i<n; i++) {
      s=(symbolList)malloc(sizeof(SNode));
      strcpy(s->name, a[i][0].c_str());
      strcpy(s->type, a[i][1].c_str());
      strcpy(s->value, a[i][2].c_str());
      r->next = s;
      r=s;
    }

    r->next=NULL;
  }

  void printList () {
    symbolList p = stable->next; // traverse pointer
    if (p == NULL) {
      cout<<"symTable is uninitialized!"<<endl;
    }
    cout<<endl;
    while (p != NULL) {
      cout<<"name: "<<p->name<<"   type: "<<p->type<<"   value: "<<p->value<<endl;
      p=p->next;
    }
    cout<<endl;
  }

  void insertList (string a[]) { // insert after the header
    symbolList p = NULL;
    p = (symbolList)malloc(sizeof(SNode));
    p->next = stable->next;

    strcpy(p->name, a[0].c_str());
    strcpy(p->type, a[1].c_str());
    strcpy(p->value, a[2].c_str());
    
    stable->next=p;
  }

  void setValue (string tp, string new_value, int updateALL) {
    symbolList p = stable->next;
    while (strcmp(tp.c_str(), p->type)) {
      p=p->next;
    }
    if (p == NULL) {
      cout<<"ERROR: cannot find the type " << tp<<endl;
      return;
    }
    if (updateALL) {
      strcpy (p->type, new_value.c_str());
    }
    strcpy(p->value,new_value.c_str());
  }

  int search (string tp) {
    symbolList p = stable->next;
    while (p) {
      if (!strcmp(tp.c_str(), p->type)) {
	return 1;
      }
      p=p->next;
    }
    return 0;
  }
  
  void getName (string tp, char * buffer) {
    symbolList p = stable->next;
    while (p) {
      if (!strcmp(tp.c_str(), p->type)) {
        strcpy(buffer, p->name);
	return;
      }
      p=p->next;
    }
  }

  void outInit () {
    symbolList p = stable->next;
    ofstream outdef;
    outdef.open("mir.c");

    outdef<<"void foo (int N) {"<<endl
	  <<"float A[N], B[N], C[N];"<<endl<<endl;

    while (p) {
      if (!strcmp(p->type, "i") || !strcmp(p->type, "N")) {
	outdef<<"int"<<" "<<p->name<<";"<<endl;
      } else // need to optimize
	if (!strcmp(p->type, "addr_A") || !strcmp(p->type, "addr_B") || !strcmp(p->type, "addr_C")) {
	  outdef<<"char *"<<" "<<p->name<<";"<<endl;
	} else {
	  outdef<<p->type<<" "<<p->name<<";"<<endl;
	}
      
      p=p->next;
    }
    outdef.close();
  }
};

// TAC
typedef struct TACNode // 9 attributes
{
  char info[40]; // concise comment, single expression: if (t0) goto end;
  
  char result[10]; // tx, ftx
  char arrow[10]; // <-
  char left_op[10]; // left_operand
  char op[10]; // +, -, *, /, addr, Ld, St
  char right_op[10]; // right_operand, including *tx or *ftx,  single value assignment: t1 <- 0
  char semicolon[10]; // ;

  int cost; // CPU cost
  int block_id; // reservation for task 2

  struct TACNode *next;
} *tacList;

class TAC
{
private:
  tacList tacTable;

public:
  TAC () {
    tacList L = (tacList)malloc(sizeof(TACNode));
    L->next=NULL;
    tacTable = L;
  }

  TAC (string a[][TAC_NUM], int n) {
    createList (tacTable, a, n);
    //    printList();
  }

  void createList (tacList &L, string a[][TAC_NUM], int n) {
    int i;
    tacList s=NULL, r=NULL; // temp
    L = (tacList)malloc(sizeof(TACNode));

    L->next = NULL; // header node
    r = L;

    for (i=0; i<n; i++) {
      s=(tacList)malloc(sizeof(TACNode));

      strcpy(s->info, a[i][0].c_str());
      strcpy(s->result, a[i][1].c_str());
      strcpy(s->arrow, a[i][2].c_str());
      strcpy(s->left_op, a[i][3].c_str());
      strcpy(s->op, a[i][4].c_str());
      strcpy(s->right_op, a[i][5].c_str());
      strcpy(s->semicolon, a[i][6].c_str());
      s->cost = atoi(a[i][7].c_str());
      s->block_id = atoi(a[i][8].c_str()); // here, all will be -1

      r->next = s;
      r=s;
    }

    r->next=NULL;
  }

  void printList () {
    tacList p = tacTable->next; // traverse pointer
    if (p == NULL) {
      cout<<"tacTable is uninitialized!"<<endl;
    }
    cout<<endl;

    ofstream out;
    if (p->block_id == 0) {
      out.open("lir.c");
      out<<endl<<"foo () {"<<endl;
    } else {
      out.open("mir.c", ios::app);
    }
    cout<<p->info<<endl; // {
    //    out<<p->info<<endl;
    p=p->next;
    while (p->next != NULL) {
      if (strcmp(p->info, "#")) {
	cout<<"\n    "<<p->info<<endl;
	out<<"\n    "<<p->info<<endl;
	p=p->next;
	continue;
      }

      if (!strcmp(p->right_op, "i")) {
	p=p->next;
	continue;
      }

      if (!strcmp(p->op, "St")) { // store deals with individually
	cout<< "    " << p->result << " " << p->arrow << " " << p->op << " " << p->right_op<<";"<<endl;
	out<< "    "<<"*((float *) "<<p->result<<")" << " = " << p->right_op<<";"<<endl;
      } else {
	cout<< "    " << p->result << " " << p->arrow << " ";
	out<< "    " << p->result << " " << "=" << " ";
      
	if (strcmp(p->left_op, "#")) { // may dont have left operand
	  cout<<p->left_op<<" ";
	  out<<p->left_op<<" ";
	}
      
	if (strcmp(p->op, "#")) { // has a operation
	  if (!strcmp(p->op, "addr")) {
	    out<<"(char *)";
	  } else if (!strcmp(p->op, "Ld*")) {
	    cout<<p->op<<" ";
	    out<<"*((float *) ";
	  } else {
	    cout<<p->op<<" ";
	    out<<p->op<<" ";
	  }
	}

	if (!strcmp(p->op, "Ld*")) {
	  out<<p->right_op<< ")"<< p->semicolon << endl;
	} else {
	  out<<p->right_op<< p->semicolon << endl;
	}
	cout<<p->right_op<< p->semicolon << endl;
      }
      
      p=p->next;
    }

    cout<<endl<< p->info <<endl; // ideally, it's "}"
    out<<endl<< p->info <<endl;
    cout<<endl;
    out.close();
  }

  void insertList (string a[]) { // insert at the tail position
    tacList p = tacTable -> next, s=NULL;
    while (p->next != NULL) {
      p=p->next;
    }

    s = (tacList)malloc(sizeof(TACNode));
    s->next = NULL;

    strcpy(s->info, a[0].c_str());
    strcpy(s->result, a[1].c_str());
    strcpy(s->arrow, a[2].c_str());
    strcpy(s->left_op, a[3].c_str());
    strcpy(s->op, a[4].c_str());
    strcpy(s->right_op, a[5].c_str());
    strcpy(s->semicolon, a[6].c_str());
    s->cost = atoi(a[7].c_str());
    s->block_id = atoi(a[8].c_str()); // here, all will be -1

    p->next=s;
  }
  
  void insert(tacList w, string a[]) { // insert at the any place
    tacList s = (tacList)malloc(sizeof(TACNode));
    s->next = w->next;

    strcpy(s->info, a[0].c_str());
    strcpy(s->result, a[1].c_str());
    strcpy(s->arrow, a[2].c_str());
    strcpy(s->left_op, a[3].c_str());
    strcpy(s->op, a[4].c_str());
    strcpy(s->right_op, a[5].c_str());
    strcpy(s->semicolon, a[6].c_str());
    s->cost = atoi(a[7].c_str());
    s->block_id = atoi(a[8].c_str());

    w->next=s;
  }

  void calcCost () {
    tacList p = tacTable->next;
    int cost=0;
    while (p) {
      cost += p->cost;
      p=p->next;
    }
    
    cout<<"CPU cost is "<<cost<<" cycles."<<endl<<endl;
  }

  void fixBugOf_i(char *tmp_tx) {
    tacList p = tacTable->next;
    while (strcmp(p->result, tmp_tx)) {
      p=p->next;
    }
    strcpy(p->right_op, "0");
  }
  
  bool isInVector (int elem, std::vector<int> vec) {
    for (vector<int>::iterator i=vec.begin();
         i!=vec.end(); i++) {
      if (elem == (*i)) {
	return true;
      }
    }
    return false;
  }

  void getLinePointer (int line, tacList &p) {
    while (line != p->block_id) {
      p = p->next;
    }
  }
  
  void varsInStmt(int line, int vars[]) {
    //int vars[3] = {0, 0, 0}; // three tx at most in one stmt
    tacList p = tacTable->next;
    p->block_id = 0; // for generating lir.c
    getLinePointer(line, p);

    char tmp_result[10], tmp_left_op[10], tmp_right_op[10];
    strcpy(tmp_result, p->result);
    strcpy(tmp_left_op, p->left_op);
    strcpy(tmp_right_op, p->right_op);

    if (tmp_result[0] == 't') { // 0
      string s_result(&tmp_result[0], &tmp_result[strlen(tmp_result)]);
      int int_result = atoi(s_result.substr(1).c_str());
      vars[0] = int_result;
    }

    if (tmp_left_op[0] == 't') { // 1
      string s_left_op(&tmp_left_op[0], &tmp_left_op[strlen(tmp_left_op)]);
      int int_left_op = atoi(s_left_op.substr(1).c_str());
      vars[1] = int_left_op;
    }

    if (tmp_right_op[0] == 't') { // 2
      string s_right_op(&tmp_right_op[0], &tmp_right_op[strlen(tmp_right_op)]);
      int int_right_op = atoi(s_right_op.substr(1).c_str());
      vars[2] = int_right_op;
    }

  }

  std::vector<int> realSpill(int var[], std::vector<int> reg, bool interORnot) {
    // actually is the intersection of two
    std::vector<int> vars;
    for (int i=0; i<3; i++) {
      if (var[i] != 0) {
	vars.push_back(var[i]);
      }
    }
    sort(vars.begin(), vars.end());
    sort(reg.begin(), reg.end());
    
    std::vector<int> v(10);
    vector<int>::iterator it = set_intersection(vars.begin(), 
						vars.end(), 
						reg.begin(), 
						reg.end(), 
						v.begin());
    int size = it - v.begin();
    std::vector<int> inter; // intersection of vars in stmt and in regs
    for (int i=0; i<size; i++) {
      inter.push_back(v[i]);
    }

    if (interORnot) {return inter;}

    std::vector<int> real_spill;
    for (vector<int>::iterator r=vars.begin();
	 r!=vars.end(); r++) {
      if (!isInVector((*r), inter) && (*r)!=0) {
	real_spill.push_back(*r);
      }
    }

    sort(real_spill.begin(), real_spill.end());
    return real_spill;
  }

  std::vector<int> spillChoose(int var[], std::vector<int> real_spill) {
    std::vector<int> curr_reg;
    std::vector<int> Choices;
    for (int i=1; i<=Reg_tx[0][0]; i++) {
      curr_reg.push_back(Reg_tx[i][0]);
    }
    std::vector<int> inter_vars_reg = realSpill(var, curr_reg, true);

    // spill choice rule:
    // 1) regs that current stmt's tx using cannot be chosed (inter_vars_reg)
    // 2) regs that other spilling candidates chosed cannot be chosed
    // 3) choose the availablities which have hightest times of being used,
    //    which can be compared through Reg_tx[][1]

    int max = Reg_tx[0][1];
    int which = 0;
    std::vector<int> tmp_tx_choice;
    int tmp_tx_which = 0;
    for (vector<int>::iterator itv=real_spill.begin();
         itv!=real_spill.end(); itv++) {
      for (int i=1; i<=Reg_tx[0][0]; i++) {
	if (!isInVector(Reg_tx[i][0], inter_vars_reg) 
	    && !isInVector(Reg_tx[i][0], tmp_tx_choice)) {
	  if (Reg_tx[i][1] > max) {
	    max = Reg_tx[i][1];
	    tmp_tx_which = Reg_tx[i][0];
	    which = i;
	  }
	}
      } // sub for
      tmp_tx_choice.push_back(tmp_tx_which);
      Choices.push_back(which);
      which=0;
      tmp_tx_which=0;
      max = Reg_tx[0][1];
    } // outmost for

    return Choices;
  } // function end 

  void mir2lir(int line, std::vector<int> spill) {
    tacList p = tacTable->next;
    getLinePointer(line, p);
    int vars[3]={0, 0, 0};
    varsInStmt(line, vars);
    
    // test
    cout<<endl<<"but line "<<line<<" uses: ";
    for (int i=0; i<3; i++) {
      if (vars[i]) {
	cout<<"t"<<vars[i]<<" ";
      }
    }
    
    // find the real_spill tx for one stmt
    // because not every tx should be spilled have to be
    // spilled in every stmt, the one shows in stmt, the one needs
    std::vector<int> reg;
    for (int j=1; j<=Reg_tx[0][0]; j++) {
      reg.push_back(Reg_tx[j][0]);
    }
    std::vector<int> real_spill = realSpill(vars, reg, false);
    std::vector<int> copy_rs;
    int index=0;
    for (vector<int>::iterator i=real_spill.begin(); i!=real_spill.end(); i++) {
      copy_rs.push_back(*i);
    }

    for (vector<int>::iterator i=copy_rs.begin(); i!=copy_rs.end(); i++, index++) {
      if (checkFull(Reg_tx[0][0])) {
	break;
      }

      for (int j=1; j<=Reg_tx[0][0]; j++) {
	if ((*i) == Reg_tx[j][0]) { // already exists
	  break;
	}
	if (Reg_tx[j][2]!=1) {
	  Reg_tx[j][0] = (*i);
	  Reg_tx[j][1] = 1;
	  Reg_tx[j][2] = 1;
	  real_spill.erase(real_spill.begin() + index);
	  break;
	}
      }
    }

    // we use vars and spill to modify the TAC
    // first, use Ri to replace original tx in stmt
    for (int m=0; vars[m]!=0 && m<3; m++) {
      char buffer[10]="Reg_";
      char tmp_num[10];
      for (int j=1; j<=Reg_tx[0][0]; j++) {
	if (vars[m] == Reg_tx[j][0]) {
	  itoa(j, tmp_num);
	  strcat(buffer, tmp_num);

	  if (m==0) {
	    strcpy(p->result, buffer);
	  } else if (m==1) {
	    strcpy(p->left_op, buffer);
	  } else if (m==2) {
	    strcpy(p->right_op, buffer);
	  }
	  break;
	} // end outmost if
      } // end submost for
    } // end outmost for    
    
    // for the list of spilling from loop body (line 6)
    // 1) choose which register to spill
    // 2) put content of reg into Mem_x
    // 3) modify Reg_tx[][]
    // 4) generate new entry for tac Mem_x <- Rx
    // 5) restore Reg_tx[][]
    // 6) get content from Mem_x back into reg
    // 7) generate new entry for tac Rx <- Mem_x

    std::vector<int> spill_choice = spillChoose(vars, real_spill); // spilling into Memory

    // modify TAC and restore
    if (spill_choice.size() == 0) {goto here;}
    for (int i=0; line>5 && i<real_spill.size() &&
	   real_spill.size()>0 && spill_choice.size()>0 && i<spill_choice.size(); i++) {
      tacList p = tacTable->next;
      if (line == 6) { // have to get the pointer prior to stmt
	getLinePointer(-10, p); // loop: label
      } else {
	getLinePointer(line-1, p);
      }
      
      char header_R[10] = "Reg_";
      char header_tx[10] = "t";
      char buffer[10];
      itoa(spill_choice[i], buffer);
      strcat(header_R, buffer);
      string send2Mem[9] = {"#", "Mem", "<-", "#", "St", header_R, ";", "20", "-1"};
      insert(p, send2Mem);
      p=p->next;
      itoa(real_spill[i], buffer);
      strcat(header_tx, buffer);
      string tx2Reg[9] = {"#", header_R, "<-", "#", "Ld*", header_tx, ";", "20", "-1"};
      insert(p, tx2Reg);

      getLinePointer(line, p);
      if (!strcmp(p->result, header_tx)) {
	strcpy(p->result, header_R);
      }
      if (!strcmp(p->left_op, header_tx)) {
        strcpy(p->left_op, header_R);
      }
      if (!strcmp(p->right_op, header_tx)) {
        strcpy(p->right_op, header_R);
      }
      
      string reg2tx[9] = {"#", header_tx, "<-", "#", "St", header_R, ";", "20", "-1"};
      insert(p, reg2tx);
      p=p->next;
      string mem2Reg[9] = {"#", header_R, "<-", "#", "Ld*", "Mem", ";", "20", "-1"};
      insert(p, mem2Reg);
      
    } // outmost for

  here:;
  }
};
/////////////////// variable table ////////////////
string initST[][3] = {{"t1", "N", "N"}};
symTable st(initST, 1);
string initTAC[][9] = {{"foo() {", "#", "#", "#", "#", "#", "#", "0", "-1"},
		       //{"float A[N], B[N], C[N];", "#", "#", "#", "#", "#", "#", "0", "-1"},
		       //{"int i;", "#", "#", "#", "#", "#", "#", "0", "-1"},
		       {"/* init the loop index */", "#", "#", "#", "#", "#", "#", "0", "-1"},
		       {"#", "t1", "<-", "#", "#", "N", ";", "1", "1"}}; // first assginment
TAC tac(initTAC, 3);
string tmp_tac_global[9] = {"#", "#", "<-", "#", "#", "#", ";", "0", "-1"};

int tmp_op; // =:0, <:1, 
char count_tx[10];
int tmp_count_flag=1; // lock
char tmp_name[20];
char tmp_value[20];
int preLock=1;
int tmp_main_op; // 0:+, 1:-, 2:*, 3:/
string op[4] = {"+", "-", "*", "/"};
char tmp_tx[5]="t";
char tmp_ftx[5]="ft";
int tmp_tac_lock=1; // once 0, means will generate a tac in two steps
char tmp_tac_left[10];
char tmp_tac_right[10];


int i=0; // ignore
int res_lock; // next one is the result A = B + C
char res_tx[10];
int couple=1;
char tempABC[5];
char res_addr[10];

char res_store[10];
int main_op_lr=0; // 0 is left, 1 is right. e.g. ft1 + ft2
char ftx_left[10];
char ftx_right[10];

int main_op_lock;
///////////////////////////////////////////////////
void itoa (int num, char * ch) {
  int i=0, j;
  char tmp;
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

/////////////////// traverse class ////////////////
class preTraverse : public AstPrePostProcessing
{
public:
  virtual void preOrderVisit(SgNode* n);
  virtual void postOrderVisit(SgNode* n);
};

void preTraverse :: preOrderVisit(SgNode* n)
{
  if (preLock) { // before for statement
    if (isSgInitializedName(n) != NULL) { //done!
      SgInitializedName* initName = isSgInitializedName(n);
      strcpy(tmp_value, initName->get_name().str());
      // insert new symbol table entry
      char tmp_tx[5]="t";
      char buffer[5];
      char tmp_type[20]="addr_";
      itoa (st.tx, buffer);
      st.updateTX();
      strcat(tmp_tx, buffer);
      strcat(tmp_type, tmp_value);
      string tmp_entry[3] = {tmp_tx, tmp_type, tmp_value};

      char tmp_assgin[5];
      if (!strcmp(tmp_value, "i")) {
	itoa(assign_i, tmp_assgin);
	string tmp_tac[9] = {"#", tmp_tx, "<-", "#", "#", tmp_value, ";", "1", tmp_assgin};
	assign_i++;
	tac.insertList(tmp_tac);
      } else {
	itoa(assign_i, tmp_assgin);
	string tmp_tac[9] = {"#", tmp_tx, "<-", "#", "addr", tmp_value, ";", "1", tmp_assgin};
	assign_i++;
	tac.insertList(tmp_tac);
      }

      st.insertList(tmp_entry);
    }

    if (isSgPlusPlusOp(n) != NULL) {
      tmp_count_flag = 0;
    }

    if (tmp_count_flag == 1) {
      if (isSgVarRefExp(n) != NULL) {
	//cout<<" "<<isSgVarRefExp(n)->get_symbol()->get_name().str()<<endl;
	strcpy(tmp_name, isSgVarRefExp(n)->get_symbol()->get_name().str());
	char tmp_typex[20]="addr_";
	strcat(tmp_typex, tmp_name);
	if (st.search(tmp_typex)) {
	  //cout<<tmp_name<<" is in list"<<endl;
	  st.setValue(tmp_typex, tmp_name, 1);
	}
	
	char tmp_txx[10];
	st.getName(tmp_name, tmp_txx);

	if (tmp_op==1) {
	  if (tmp_tac_lock == 1) {
	    strcpy(tmp_tac_right, tmp_txx);	    
	  }
	  if (tmp_tac_lock == 0) {
            strcpy(tmp_tac_left, tmp_txx);
	    // update symbol
	    char tmp_tx[5]="t";
	    char buffer[5];
	    itoa (st.tx, buffer);
	    st.updateTX();
	    strcat(tmp_tx, buffer);
	    
	    string tmp_entry[3] = {tmp_tx, "int", "#"};
	    st.insertList(tmp_entry);
	    // update tac

	    char tmp_assgin1[5];
	    itoa(assign_i, tmp_assgin1);
	    string tmp_tac[9] = {"#", tmp_tx, "<-", tmp_tac_left, "-", tmp_tac_right, ";", "1", tmp_assgin1};
	    assign_i++;
	    tac.insertList(tmp_tac);
	    
	    char ifgoto_buffer[40]="if (";
	    strcat(ifgoto_buffer, tmp_tx);
	    strcat(ifgoto_buffer, ") goto end;");

	    string tmp_if_tac[9] = {ifgoto_buffer, "#", "#", "#", "#", "#", ";", "4", "-100"}; // goto
	    tac.insertList(tmp_if_tac);
	    
	    tmp_op=-1;
	  }
	  tmp_tac_lock=0;
	}
      }
    }
    
    if (tmp_count_flag == 0){      
      if (isSgVarRefExp(n) != NULL) {
	//cout<<isSgVarRefExp(n)->get_symbol()->get_name().str()<<" is count var!"<<endl;
	strcpy(count_tx, isSgVarRefExp(n)->get_symbol()->get_name().str());
	char tmp_count[10];
	st.getName(count_tx, tmp_count);
	strcpy (count_tx, tmp_count); // reduction variable
	tmp_count_flag=-1;
	preLock=0;
      }
    }

    if (isSgAssignOp(n) != NULL) { // could be deleted!!! here!!!
      tmp_op = 0; // now, you know it's for initialization
    }

    if (isSgIntVal(n) != NULL) {
      st.setValue(tmp_name, "0", 0);
      st.getName(tmp_name, tmp_tx);
      tac.fixBugOf_i(tmp_tx);
    }
    
    if (isSgLessThanOp(n) != NULL) {
      tmp_op = 1;
      string label[9]={"loop:", "#", "#", "#", "#", "#", "#", "0", "-10"}; // loop
      tac.insertList(label);
    }
  }

  if (!preLock) {
    if (isSgAssignOp(n) != NULL) {
      tmp_op = 0; // =
      cout<<" = ";
    }
 
    if(isSgVarRefExp(n) != NULL) {
      if (i<1) {i++; return;} // ignore first i
      strcpy(tmp_name, isSgVarRefExp(n)->get_symbol()->get_name().str());

      if (couple==1) {
	char tmp_type[20]="addr_";
	char tmp_tx[10];
	strcpy (tempABC, tmp_name);
	strcat (tmp_type, tmp_name); // e.g. addr_A
	st.getName(tmp_type, tmp_tx); // ideally tmp_tx is t3
	strcpy(res_addr, tmp_tx);
	if (tmp_op == 0) {
	  strcpy(res_tx, tmp_tx);
	}
	couple=0;
	return;
      }
      if (couple==0) { // i
	char load_buffer[40]="/* load ";
	strcat(load_buffer, tempABC);
	strcat(load_buffer, " */");
	string load[9] = {load_buffer, "#", "#", "#", "#", "#", ";", "0", "-1"};
	tac.insertList(load);
	char tmp_left[10];
	st.getName(tmp_name, tmp_left); // ideally tmp_left is t2

	// update symbol table
	char tmp_tx[5]="t";
	char buffer[5];
	itoa (st.tx, buffer);
	st.updateTX();
	strcat(tmp_tx, buffer);
	string tmp_entry[3] = {tmp_tx, "int", "#"}; // tmp_tx is left result
	st.insertList(tmp_entry);

	// update tac
	char tmp_assgin2[5];
	itoa(assign_i, tmp_assgin2);
	string calc[9] = {"#", tmp_tx, "<-", tmp_left, "*", "8", ";", "1", tmp_assgin2};
	assign_i++;
	tac.insertList(calc);

	char load_tx[5]="t";
	char buffer2[5];
	itoa (st.tx, buffer2);
	st.updateTX();
	strcat(load_tx, buffer2);
	string tmp_entry2[3] = {load_tx, "char *", "#"};
	st.insertList(tmp_entry2);
	
	char tmp_assgin3[5];
	itoa(assign_i, tmp_assgin3);
	string loadABC[9] = {"#", load_tx, "<-", res_addr, "+", tmp_tx, ";", "1", tmp_assgin3};
	assign_i++;
	tac.insertList(loadABC);

	if (tmp_op==0) {
	  strcpy(res_store, load_tx);
	  tmp_op=1;
	} else {
	  // generate ftx
	  char tmp_ftx[5]="ft";
	  char buffer3[5];
	  itoa (st.ftx, buffer3);
	  st.updateFTX();
	  strcat(tmp_ftx, buffer3);
	  string tmp_entry3[3] = {tmp_ftx, "float", "#"}; // tmp_tx is left result     
	  st.insertList(tmp_entry3);

	  char tmp_assgin4[5];
	  itoa(assign_i, tmp_assgin4);
	  string loadftx[9] = {"#", tmp_ftx, "<-", "#", "Ld*", load_tx, ";", "20", tmp_assgin4};
	  assign_i++;
	  tac.insertList(loadftx);

	  if (main_op_lr==0) { // left
	    strcpy (ftx_left, tmp_ftx);
	    main_op_lr=1;
	  } else { // also means one A = B ? C is done
	    strcpy (ftx_right, tmp_ftx);

	    char tmp_ftx_final[5]="ft";
	    char buffer4[5];
	    itoa (st.ftx, buffer4);
	    st.updateFTX();
	    strcat(tmp_ftx_final, buffer4);
	    string tmp_entry4[3] = {tmp_ftx_final, "float", "#"}; // tmp_tx is left result
	    st.insertList(tmp_entry4);
	    
	    string newline[9] = {"/* Store */", "#", "#", "#", "#", "#", ";", "0", "-1"};
	    tac.insertList(newline);

	    char tmp_assgin5[5];
	    itoa(assign_i, tmp_assgin5);
	    string calcftx[9] = {"#", tmp_ftx_final, "<-", ftx_left, op[tmp_main_op], ftx_right, ";", "20", tmp_assgin5};
	    assign_i++;
	    tac.insertList(calcftx);

	    char tmp_assgin6[5];
	    itoa(assign_i, tmp_assgin6);
	    string store[9] = {"#", res_store, "<-", "#", "St", tmp_ftx_final, ";", "20", tmp_assgin6};
	    assign_i++;
            tac.insertList(store);
	    main_op_lr=0;
	  }
	}
	
	couple=1;
      }

    }

    if(isSgAddOp(n) != NULL) {
      tmp_main_op=0;
      main_op_lock=0;
    }

    if(isSgSubtractOp(n) != NULL) {
      tmp_main_op=1;
      main_op_lock=0;
    }

    if(isSgMultiplyOp(n) != NULL) {
      tmp_main_op=2;
      main_op_lock=0;
    }

    if(isSgDivideOp(n) != NULL) {
      tmp_main_op=3;
      main_op_lock=0;
    }
  }
}

void preTraverse :: postOrderVisit(SgNode * n) {}
/////////////////////traverse end/////////////////

void reg_alloc(std::vector<int> outVars, int num, int line) {
  int j;
  std::vector<int> remains;
  std::vector<int> news;
  
  resetRegTx(2);
  
  // find the identical tx in current registers
  vector<int>::iterator i=outVars.begin();
  i++;
  for (; i!=outVars.end(); i++) { // outVars[0] is not I want
    for (j=1; j<=num; j++) {
      if ((*i)==Reg_tx[j][0]) { 
	Reg_tx[j][2] = 1; // Rj remains its live variable
	Reg_tx[j][1]++;
	remains.push_back((*i));
	break;
      }
    }
  }

  vector<int>::iterator p=outVars.begin();
  p++;
  for (; p!=outVars.end(); p++) {
    bool flag = true;
    for (vector<int>::iterator q=remains.begin();
	 q!=remains.end(); q++) {
      if ((*p) == (*q)) {
	flag = false;
      }
    }
    if (flag) {
      news.push_back((*p));
    }
  }
  
  // find the unused register or recycled register
  for (i=news.begin(); i!=news.end(); i++) {
    if (checkFull(num)) {
      cout<<endl<<"no registers to use, must spilling: ";
      break;
    }
    
    for (j=1; j<=num; j++) {
      if (Reg_tx[j][2]!=1) {
	Reg_tx[j][0] = (*i);
	Reg_tx[j][1] = 1;
	Reg_tx[j][2] = 1;
	break;
      }
    }
  }

  // maybe from i we need spilling
  std::vector<int> spill;
  for (; i!=news.end(); i++) {
    spill.push_back((*i));
    cout<<"t"<<(*i)<<" ";
  }

  // modifiy the TAC
  tac.mir2lir(line, spill);
  
  // test
  cout<<endl;
  cout<<endl<<"---- registers (t0 means empty)----"<<endl;
  for (int k=1; k<=num; k++) {
    cout<<"Reg_"<<k<<"   "<<"t"<<Reg_tx[k][0]<<endl;
  }
  cout<<endl;
}
///////////////// liveness mir.c /////////////////
///////////////// register alloction /////////////
////////// including coloring and spilling ///////
void livenessAnalysis(SgProject *project, int num_of_reg) { // by default we analyze "foo" function
  // set Reg_tx array to 0
  resetRegTx();
  Reg_tx[0][0]=num_of_reg; // for convenience
  cout<<"We only have "<<num_of_reg<<" registers!"<<endl<<endl;
  
  bool flag=false, error=false;
  DFAnalysis *defuse = new DefUseAnalysis(project);
  defuse->run(flag);
  LivenessAnalysis *liveness = new LivenessAnalysis(flag,(DefUseAnalysis *)defuse);

  std::vector <FilteredCFGNode <IsDFAFilter> > dfaFunctions;
  NodeQuerySynthesizedAttributeType defs = NodeQuery::querySubTree(project, V_SgFunctionDefinition);
  
  for (NodeQuerySynthesizedAttributeType::const_iterator i = defs.begin();
       i!=defs.end(); i++) {
    SgFunctionDefinition * funcdef = isSgFunctionDefinition(*i);
    //std::string name = funcdef->class_name();
    //string funcName = funcdef->get_declaration()->get_qualified_name().str();
    FilteredCFGNode <IsDFAFilter> rem_source = liveness->run(funcdef,error);
    /*if (error) { // this for debug
      break;
      }*/
    NodeQuerySynthesizedAttributeType assigns = NodeQuery::querySubTree(funcdef, V_SgAssignOp);
    
    // nodesList contains every assginment statement
    cout<<"Live variables of "<<assign_i<<" assignments:"<<endl;
    int ass=1;
    for (NodeQuerySynthesizedAttributeType::const_iterator nodesList = assigns.begin();
	 nodesList!=assigns.end(); nodesList++) {
      SgNode* node = *nodesList;
      ROSE_ASSERT(node);
      //std::vector<SgInitializedName*> in = liveness->getIn(node);
      std::vector<SgInitializedName*> outList = liveness->getOut(node);

      //std::vector<string> outName;   
      //itv = out.begin();

      // this will output each assginment stmt's out variables
      int tx_i=0; // number of tx for each assginment
      std::vector<int> outVars;
      outVars.push_back(-1000); // outVars[0] should be the number of tx in one stmt

      cout<<"-------- Line "<<ass<<" --------"<<endl;
      cout<<"outlist: ";
      for (std::vector<SgInitializedName*>::const_iterator j = outList.begin();
	   j!=outList.end(); j++) {
	SgInitializedName* init = *j;
	string tmp_var = init->get_name();
	if (tmp_var.find_first_of('t', 0)==0) { // all we care is symbolic vars
	  cout<<tmp_var<<" ";
	  tx_i++;
	  // extract the integer number of tx
	  int tx_label = atoi(tmp_var.substr(1).c_str());
	  outVars.push_back(tx_label);
	}
      }
      sort(outVars.begin(), outVars.end());
      outVars[0]=tx_i;
      // do the register allocation for one ass-stmt
      reg_alloc(outVars, num_of_reg, ass);

      ass++;
      cout<<endl;
      
    }

  }
}
/////////////////// my own class end//////////////
//////////////////////////////////////////////////
int
main (int argc, char *argv[])
{
  int num_of_reg;

  /* generate DOT graph for the AST */
  CppToDotTranslator dotc;
  dotc.translate(argc,argv);
 
  /* generate the AST in pdf format */ 

  CppToPdfTranslator pdfc;
  pdfc.translate(argc,argv);
  
  SgProject *project = frontend (argc, argv);
  ROSE_ASSERT (project != NULL);

  /* find the function definition (unique in this project) */
  Rose_STL_Container<SgNode*> funcDef =
    NodeQuery::querySubTree(project, V_SgFunctionDefinition);
  Rose_STL_Container<SgNode*>::iterator def = funcDef.begin();

  /* find the loops in the source file */
  Rose_STL_Container<SgNode*> forLoopStmtList =
    NodeQuery::querySubTree(project, V_SgForStatement);

  /* process loops one by one, in this project, the input file has only one loop */
  Rose_STL_Container<SgNode*>::iterator i = forLoopStmtList.begin();
  //SgForStatement *forstmt = isSgForStatement(*i);
  //ROSE_ASSERT(forstmt != NULL);

  /* your work here */
  /////////////////////// start /////////////////////////

  // variable initialization part
  preTraverse yjx;
  yjx.traverse(*def); // function init

  string info[9] = {"/* Update i */", "#", "#", "#", "#", "#", ";", "0", "-1"};
  tac.insertList(info);

  char tmp_assgin7[5];
  itoa(assign_i, tmp_assgin7); // here assign_i should store the number of all assigns
  string plusplus[9] = {"#", count_tx, "<-", count_tx, "+", "1", ";", "1", tmp_assgin7};
  tac.insertList(plusplus);
  string gotos[9] = {"goto loop;", "#", "#", "#", "#", "#", ";", "4", "-1"};
  tac.insertList(gotos);
  string end[9] = {"end:;", "#", "#", "#", "#", "#", ";", "0", "-1"};
  tac.insertList(end);
  string paren[9] = {"}", "#", "#", "#", "#", "#", ";", "0", "-1"};
  tac.insertList(paren);

  st.outInit();
  st.printList();
  cout<<"------ MIR ------"<<endl;
  tac.printList();
  tac.calcCost();

  /////////////////////// end ///////////////////////////

  /* after the transformation */
  //  AstTests::runAllTests(project);
  //project->unparse();
  /* output source file */ 
  //return backend (project);


  //////////////////// liveness analysis ///////////////
  ////////////////// register allocation ///////////////
  ////////////////// coloring and spilling /////////////
  strcpy(argv[1], "mir.c");
  SgProject *pro_liveness = frontend (argc, argv);
  ROSE_ASSERT (pro_liveness != NULL);
  
 again:
  cout<<endl<<"How many registers you want (must >= 5): ";
  cin>>num_of_reg;
  if (num_of_reg < 5) {goto again;}
  livenessAnalysis(pro_liveness, num_of_reg);
  // test
  cout<<"------ LIR ------"<<endl;
  tac.printList();
  tac.calcCost();
  
  project->unparse();
  pro_liveness->unparse();

  return 0;
}
