#include <iostream>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fstream>
#include <queue>
#include <iomanip>
using namespace std;

//Some random #define(s) to understanf=d things better
#define AVAILABLE true
#define NOT_AVAILABLE false
#define Filled true
#define Empty false

//ICache and Dcache arrays
unsigned __int8 Instruction[256];
unsigned __int8 DATA[256];


//Program counter and other registers needed in pipeline
unsigned __int8 PC=0;
unsigned __int8 A,B,LMD,B_ST;
queue<unsigned __int8> ALUOutput;

//Registers and whether they are avaialable or not
unsigned __int8 R[16];
int register_available[16]; //value in register_avaialable[i] denotes how many instructions are yet to write in R[i]

//Variables for keepig track of pipeline
unsigned __int16 Fetch, Decode, Execute, Mem_Acc, Write_Back;
bool Fetch_status=Empty; 
bool Decode_status=Empty; 
bool Execute_status=Empty; 
bool Mem_Acc_status=Empty; 
bool Write_Back_status=Empty; 

//Variables for Output
int exe_inst=0, arth_inst=0, log_inst=0, shift_inst=0, mem_inst=0 ,li_inst,ctrl_inst=0,halt_inst=0;
int cycles=0;
int stalls=0, data_stalls=0, control_stalls=0;
bool c_stall;
bool Finish;

//To convert the input to 8 bit number
unsigned __int8 get_deci(char c){
   switch(c){
      case 'f': return (15);
      case 'e': return (14);
      case 'd': return (13);
      case 'c': return (12);
      case 'b': return (11);   
      case 'a': return (10);
      default : return (c-'0');
   }
}

unsigned __int8 get_byte(string s){
   
   if(s.size()==0)return 0;
   if(s.size()==1)return get_deci(s[0]);
   return (((get_deci(s[0]))*16)+get_deci(s[1]));
   
}



//To convert to hexa from 8 bit for final output
string get_hexa_form(unsigned __int8 a){
   string ans;
   switch(((int)a)/16){
       case 15: ans.push_back('f'); break;
       case 14: ans.push_back('e'); break;
       case 13: ans.push_back('d'); break;
       case 12: ans.push_back('c'); break;
       case 11: ans.push_back('b'); break;
       case 10: ans.push_back('a'); break;
       default: ans.push_back(((int)a/16)+'0');
   }
    switch(((int)a)%16){
       case 15: ans.push_back('f'); break;
       case 14: ans.push_back('e'); break;
       case 13: ans.push_back('d'); break;
       case 12: ans.push_back('c'); break;
       case 11: ans.push_back('b'); break;
       case 10: ans.push_back('a'); break;
       default: ans.push_back(((int)a%16)+'0');
   }
   return ans;
}

int main(int argc, char *argv[]){
 ifstream ifile(argv[1]); //Icache file for instructions
 ifstream dfile(argv[2]); //Dcache files for Data
 ifstream rfile(argv[3]); //Registers file for registers

 for(int i=0;i<256;i++) {
   string s;
   ifile>>s;
   Instruction[i]=get_byte(s); //Read Icache and dtore in deci form
 }
 for(int i=0;i<256;i++){
   string s;
   dfile>>s;
   DATA[i]=get_byte(s); //Read Dcache and store in deci form
 }
 for(int i=0;i<16;i++){
   string s;
   rfile>>s;
   R[i]=get_byte(s); //Read registers and store in deci form
   register_available[i]=0; //Initially all registers are available to read, 
 }

 while(Write_Back/4096!=15){ //The value n write_back is not halt
    cycles++; 

    if(Write_Back_status==Filled){
       //Write back filled means that we completed the write back status in the previous stage
      //If it is filled, check what type of instruction it is and then empty the write back register
        Write_Back_status=Empty;
        exe_inst++;

        switch(Write_Back/4096){
           case 0: case 1: case 2: case 3:  arth_inst++;  break;
           case 4: case 5: case 6: case 7:  log_inst++;   break;          
           case 8: case 9:                  shift_inst++; break;
           case 10:                         li_inst++;    break;
           case 11: case 12:                mem_inst++;   break;
           case 13: case 14:                ctrl_inst++;  break;
           case 15:                         halt_inst++;  break;
           default: break;
        }
    }

    if(Mem_Acc_status==Filled&&Write_Back_status==Empty){
      //We will do write back in a cycle only if the write back is empty and Mmem_Acc is filled, i.e.,
      //We completed a Mem_Acc status in previous cycle and the write back stage is available for that stage

       Mem_Acc_status=Empty; //Empty the Mem_Acc as the instruction will be forwarded to Write Back Stage
       Write_Back_status=Filled; //Now the wite back status is filled, as we will complete the write back stage for the instruction in this cycle
       Write_Back=Mem_Acc;

       switch(Write_Back/4096){
        case 10:
        R[(Write_Back%4096)/256]=(Write_Back%256); //Load the immediate value
        register_available[(Write_Back%4096)/256]--; //The instruction has been completed and now the register is no more dependent on this instruction
        break;

        case 11: 
        R[(Write_Back%4096)/256]=LMD; //LOad the value read from memory
        register_available[(Write_Back%4096)/256]--; //Same as case 10
        break;

        case 0: case 1: case 2: case 3: case 4: case 5: 
        case 6: case 7: case 8: case 9:
         R[(Write_Back%4096)/256]=ALUOutput.front(); //Load the aluoutput
         register_available[(Write_Back%4096)/256]--; //Same as case 10
         ALUOutput.pop(); //Pop the used ALUoutput 
         break;
       }
    }

    if(Execute_status==Filled&&Mem_Acc_status==Empty){
       //If Mem_acc stage is now empty, and we completed the execute stage for that instruction, Do the mem_acc for that stage
       

        Mem_Acc_status=Filled;
        Execute_status=Empty;
        Mem_Acc=Execute;
        switch(Mem_Acc/4096){
            case 10: LMD=Execute%256; break; //Calculate LMD for LI
            case 11: LMD=DATA[ALUOutput.front()];   ALUOutput.pop(); break; //Claculate LMD by accessing Data array
            case 12: DATA[ALUOutput.front()]=B_ST; //Load the regster's value in data array  
                     ALUOutput.pop();  break; //Pop the used Aluoutput
            case 13: case 14: c_stall=false; break; //Now as PC stores the correct PC value for next istruction, now disable the control stall
            default: break; //default lite
        }
    }

    if(Execute_status==Empty&&Decode_status==Filled){
      //Same as previous checks

        Decode_status=Empty;
        Execute_status=Filled;
        Execute=Decode;

        switch(Execute/4096){
            case 0: ALUOutput.push(A+B); break;
            case 1: ALUOutput.push(A-B); break;
            case 2: ALUOutput.push(A*B); break;
            case 3: ALUOutput.push(A+1); break;
            case 4: ALUOutput.push(A&B); break;
            case 5: ALUOutput.push(A|B); break;
            case 6: ALUOutput.push(A^B); break;
            case 7: ALUOutput.push(~A); break;
            case 8: ALUOutput.push(A*pow(2,Execute%16)); break;
            case 9: ALUOutput.push(A/pow(2,Execute%16)); break;
            case 10: break;
            case 11: 
             if(Execute%16<8)ALUOutput.push(A+Execute%16);
             else ALUOutput.push(A+Execute%16-16);
             break;
            case 12:
             if(Execute%16<8)ALUOutput.push(A+Execute%16);
             else ALUOutput.push(A+Execute%16-16);
             B_ST=B; 
             break;
            case 13: PC=PC+2*(Execute%4096)/16; break;
            case 14: if(A==0)PC=PC+2*(Execute%256);  break;
            case 15: break;
        }
        //Update PC if needed or store in the ALUoutput according to the need
    } 


    if(Fetch_status==Filled&&Decode_status==Empty){
      //Same as previous checks

        Fetch_status=Empty;
        Decode_status=Filled;
        Decode=Fetch;

        switch(Decode/4096){
           case 0: case 1: case 2: case 4: case 5: case 6:
           if(register_available[(Decode%256)/16]==0&&register_available[Decode%16]==0){ 
            //Check that registers (that we have to read) are not being written in the instructions that have not been completed
             A= R[(Decode%256)/16];
             B=R[Decode%16];
             register_available[(Decode/256)%16]++; //Now this instruction will write in a register in write back stage, so update register_available accordingly
           }
           else{
            data_stalls++; //If not avaialble, then we have encountered a data stall
            Decode_status=Empty; // decode has not been completed for tht instruction, so decode remains empty
            Fetch_status=Filled; //As the last stage completed for that instruction is fetch, fetch remains filled
           }
            break;

          case 3:
          if(register_available[(Decode/256)%16]==0){ //Same as previous cases
            A=R[(Decode/256)%16];
            register_available[(Decode/256)%16]++;
          }
          else{
            data_stalls++; //Same as before
            Decode_status=Empty;
            Fetch_status=Filled;
          }
          break;

          case 7: case 8: case 9:
          if(register_available[(Decode%256)/16]==0){
            A=R[(Decode%256)/16];
            register_available[(Decode/256)%16]++;
          }
          else{
            data_stalls++;
            Decode_status=Empty;
            Fetch_status=Filled;
          }
          break;
          
          case 10:
            register_available[((Decode/256)%16)]++; //No registers are being read, but wee will load immediate in a register in later cycles
            break;

          case 11: 
           if(register_available[(Decode%256)/16]== 0){
             A=R[(Decode%256)/16];
             register_available[(Decode/256)%16]++;
           }
           else{
            data_stalls++;
            Decode_status=Empty;
            Fetch_status=Filled;
           }
           break;

           case 12:
           if(register_available[(Decode%256)/16]== 0&&register_available[(Decode/256)%16]==0){
             A=R[(Decode%256)/16];
             B=R[(Decode/256)%16];
           }
           else{
            data_stalls++;
            Decode_status=Empty;
            Fetch_status=Filled;
           }
           break;

           case 13: 
           
           c_stall=true; //Set control_stall flag as true
           break;

           case 14:
           if(register_available[(Decode/256)%16]==0){
            
            c_stall=true; //If register is ready to read, set control flag as true
            A=R[(Decode/256)%16]; //Read the register
           
           }
           else{
            data_stalls++;
            Decode_status=Empty;
            Fetch_status=Filled;
           }
           break;
           case 15:
           Finish=true; //Set finish flag as true
           break;

           default: break;
        }
    }

    if(Fetch_status==Empty&&Finish==false&&c_stall==false){ 
      //If finish and control stall flag is not 1, and Fetch stage is ready to fetch an instruction, then fetch
        Fetch_status=Filled;
        Fetch=Instruction[PC];
        Fetch*=256;
        Fetch+=Instruction[PC+1];
        PC+=2; //Increase PC by 2
    }
    else if(c_stall==true)control_stalls++; //If control stall flag is 1, increase the number of control stalls encountered
    
  
  //  cout<<Write_Back<<" "<<Mem_Acc<<" "<<Execute<<" "<<Decode<<" "<<Fetch<<endl;
 }

 exe_inst++;
 halt_inst++;

 ofstream final_data_file(argv[4]);
 ofstream final_performance_file(argv[5]);
 
 for(int i=0;i<256;i++) final_data_file<<get_hexa_form(DATA[i])<<endl;
 
 final_performance_file<<"Total number of instructions executed        : "<<exe_inst<<endl
                       <<"Number of instructions in each class"<<endl
                       <<"Arithmetic instructions                      : "<< arth_inst<<endl
                       <<"Logical instructions                         : "<<log_inst<<endl
                       <<"Shift instructions                           : "<<shift_inst<<endl
                       <<"Memory instructions                          : "<<mem_inst<<endl
                       <<"Load immediate instructions                  : "<<li_inst<<endl
                       <<"Control instructions                         : "<<ctrl_inst<<endl
                       <<"Halt instructions                            : "<<halt_inst<<endl
                       <<"Cycles Per Instruction                       : "<<(float)((cycles*1.0)/exe_inst)<<endl
                       <<"Total number of stalls                       : "<<data_stalls+control_stalls<<endl
                       <<"Data stalls (RAW)                            : " <<data_stalls<<endl
                       <<"Control stalls                               : " <<control_stalls<<endl;
    
}