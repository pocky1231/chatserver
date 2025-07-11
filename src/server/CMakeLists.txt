
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_List)
aux_source_directory(./model MODEL_List)
aux_source_directory(./redis REDIS_LIST)



#生成可执行文件
add_executable(ChatServer ${SRC_LIST} ${DB_List} ${MODEL_List} ${REDIS_LIST})

#连接动态库
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)