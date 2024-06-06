## Understanding the working of a scalar pipelined processor 
In this assignment, I tried understanding scalar pipelined processor by simulating the different components at the software level, and using the c++ language for implementation.
### How to run the code
 1. Compile the cpp file LAB8 using:
           `g++ main.cpp`
 2. After Compiling, use the command : `.\a.exe .\ICache.txt .\DCache.txt .\RF.txt .\DCache.txt .\Output.txt` <br>
     (output folder should be present in the directory before using this command)
 
### Architecture's Components' equivalents in our model
* I have declared variables for all the the 5 cycles, Fetch, Decode, Execute, Memory Access and Write Back. Along with these, I have declared 5 bool variables to check whether I have an instruction to do for that stage in a particular cycle.
* For registers, and to check whether they are being used in a previous incomplete process, I have declared an array of unsigned __int8 (unsigned char) and int respectively. The register_available array stores the number of instructions that are yet to write in a register, and when it is 0 that means no more instructions are yet to write. 
* To store ICache and Dcache, I have declared 2 unsigned __int8 arrays of 256 size, and named them as Instructions and DATA.
* For ALUOutput, I have declared a queue, so it acts like a buffer.
* For all other components, such as PC, A, B, LMD, etc. I have declared variables.

