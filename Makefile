SRC_PATH = ./src/
OBJ_PATH = ./obj/
HEADER_PATH = ./headers/
OBJ_MODULES = $(OBJ_PATH)server.o $(OBJ_PATH)client.o $(OBJ_PATH)message.o $(OBJ_PATH)user.o $(OBJ_PATH)fileSubsystem.o
TARGET = chat
CXX = g++
CXXFLAGS = -g -Wall #-D LINUX
CXX+FLAGS = $(CXX) $(CXXFLAGS)

$(TARGET): $(SRC_PATH)main.cpp $(OBJ_PATH)server.o $(OBJ_PATH)client.o $(OBJ_PATH)message.o $(OBJ_PATH)user.o $(OBJ_PATH)fileSubsystem.o
	$(CXX+FLAGS) $< $(OBJ_MODULES) -o $@

$(OBJ_PATH)message.o: $(SRC_PATH)message.cpp $(SRC_PATH)server.cpp $(HEADER_PATH)server.h
	$(CXX+FLAGS) -c $< -o $@

$(OBJ_PATH)user.o: $(SRC_PATH)user.cpp $(SRC_PATH)server.cpp $(HEADER_PATH)server.h
	$(CXX+FLAGS) -c $< -o $@

$(OBJ_PATH)fileSubsystem.o: $(SRC_PATH)fileSubsystem.cpp $(SRC_PATH)server.cpp $(HEADER_PATH)server.h
	$(CXX+FLAGS) -c $< -o $@

$(OBJ_PATH)server.o: $(SRC_PATH)server.cpp $(HEADER_PATH)server.h $(OBJ_PATH)message.o $(OBJ_PATH)user.o $(OBJ_PATH)fileSubsystem.o
	$(CXX+FLAGS) -c $< -o $@

$(OBJ_PATH)client.o: $(SRC_PATH)client.cpp $(HEADER_PATH)client.h $(HEADER_PATH)server.h
	$(CXX+FLAGS) -c $< -o $@


clean:
	rm -f obj/*.o $(TARGET)

ex_and_run:
	$(TARGET) && ./$(TARGET)

run:
	./$(TARGET)
