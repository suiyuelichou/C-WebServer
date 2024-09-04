CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

# mysql 库和头文件路径
MYSQL_INC_PATH := /usr/include/mysql # :=是简单赋值运算符，将'/usr/include/mysql'赋值给'MYSQL_INC_PATH'变量
MYSQL_LIB_PATH := /usr/lib64/mysql

# 添加包含路径
CXXFLAGS += -I$(MYSQL_INC_PATH)
LDFLAGS += -L$(MYSQL_LIB_PATH) -lpthread -lmysqlclient

server: main.cpp  ./timer/lst_timer.cpp ./http/http_conn.cpp ./log/log.cpp ./CGImysql/sql_connection_pool.cpp  webserver.cpp config.cpp
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)
# 实际上就是：g++ -o server main.cpp ./timer/lst_timer.cpp ./http/http_conn.cpp ./log/log.cpp ./CGImysql/sql_connection_pool.cpp webserver.cpp config.cpp -g -I/usr/include/mysql -L/usr/lib64/mysql -lpthread -lmysqlclient

# 或者：     g++ -o server main.cpp ./timer/lst_timer.cpp ./http/http_conn.cpp ./log/log.cpp ./CGImysql/sql_connection_pool.cpp webserver.cpp config.cpp -O2 -I/usr/include/mysql -L/usr/lib64/mysql -lpthread -lmysqlclient

clean:
	rm  -r server

