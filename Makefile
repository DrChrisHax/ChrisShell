#Makefile Template 
#Created By: Chris Manlove 

EXE = ChrisShell
CXX = @clang
CXXFLAGS = -Wall -g -O3
#LINKERFLAGS

$(EXE): $(OBJ) Makefile
		@rm -f $(EXE)
#$(CXX) *.cpp $(CXXFLAGS) $(LINKERFLAGS) -o $(EXE)
		$(CXX) *.c $(CXXFLAGS) -o $(EXE) 
clean:
		@rm -f $(EXE)
run: $(EXE)
		@./$(EXE)
