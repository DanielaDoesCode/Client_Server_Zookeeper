BIN_DIR = binary
INC_DIR = include
OBJ_DIR = object
SRC_DIR = source
LIB_DIR = lib
DEP_DIR = dependencies

CC = gcc
CFLAGS = -Wall -D THREADED -MMD -MP -MF $(DEP_DIR)/$*.d -I $(INC_DIR) -g
LIBFLAGS = -L$(LIB_DIR) -l:$(LIB).a -lprotobuf-c -lzookeeper_mt
ARFLAGS = -rcs

LIB_SRC = $(SRC_DIR)/data.c $(SRC_DIR)/entry.c $(SRC_DIR)/list.c $(SRC_DIR)/table.c
LIB_OBJ = $(patsubst $(SRC_DIR)%.c,$(OBJ_DIR)%.o,$(LIB_SRC))

TARGET_OBJ = $(filter-out $(LIB_OBJ), $(wildcard $(OBJ_DIR)/*.o))

LIB = libtable

TABLE_CLIENT = $(BIN_DIR)/table_client
TABLE_SERVER = $(BIN_DIR)/table_server

CLIENT_SRC = $(SRC_DIR)/network_client.c $(SRC_DIR)/client_stub.c $(SRC_DIR)/sdmessage.pb-c.c $(SRC_DIR)/table_client.c $(SRC_DIR)/message.c
CLIENT_OBJ = $(patsubst $(SRC_DIR)%.c,$(OBJ_DIR)%.o,$(CLIENT_SRC))

SERVER_SRC = $(SRC_DIR)/network_server.c $(SRC_DIR)/table_skel.c $(SRC_DIR)/sdmessage.pb-c.c $(SRC_DIR)/table_server.c $(SRC_DIR)/message.c $(SRC_DIR)/client_stub.c $(SRC_DIR)/network_client.c
SERVER_OBJ = $(patsubst $(SRC_DIR)%.c,$(OBJ_DIR)%.o,$(SERVER_SRC))


all: libtable table-client table-server 

table-client: $(TABLE_CLIENT)

table-server: $(TABLE_SERVER)

libtable: $(LIB_OBJ)
	ar $(ARFLAGS) $(LIB_DIR)/libtable.a $(LIB_OBJ)

$(TABLE_CLIENT): libtable $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $(TABLE_CLIENT) $(CLIENT_OBJ) $(LIBFLAGS)

$(TABLE_SERVER): libtable $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $(TABLE_SERVER) $(SERVER_OBJ) $(LIBFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c 
	$(CC) $(CFLAGS) -c $< -o $@

$(SRC_DIR)/sdmessage.pb-c.c: sdmessage.proto
	protoc-c --c_out=. sdmessage.proto
	mv sdmessage.pb-c.c $(SRC_DIR)
	mv sdmessage.pb-c.h $(INC_DIR)

include $(wildcard $(DEP_DIR)/*.d)

clean:
	rm -rf $(TARGET_OBJ) $(BIN_DIR)/* $(LIB_DIR)/* $(DEP_DIR)/*