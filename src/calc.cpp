#include <iostream>
#include <zmq.hpp>
#include <zmq_addon.hpp>

#include "topology.hpp"

std::string adr(int id) {
    return "tcp://127.0.0.1:" + std::to_string(50000 + id);
}

int main(int argc, char *argv[]) {
    int my_id = std::atoi(argv[1]); // id, на основе которого будем посылать сообщения "вверх"
    int child_id = -1; // id следующего узла, на основе которого будем передавать сообщения дальше
    
    zmq::context_t context(1);
    zmq::socket_t parent_socket(context, ZMQ_REP); // сокет для общения с родительскими узлами
    parent_socket.connect(adr(my_id));
    zmq::socket_t child_socket(context, ZMQ_REQ); // сокет для общения с дочерними узлами

    while (1) {
        zmq::multipart_t msg, reply, passing; // сообщения из нескольких частей
        msg.recv(parent_socket);
        int dest_id = stoi(msg.peekstr(0));
        std::string command = msg.peekstr(1);
        if(command == "heartbit") {
            if (child_id != -1) {
                int TIME = stoi(msg.peekstr(2));
                int got_reply = 0;
                for (int tr=0; tr < 4; tr++) {
                    msg.send(child_socket);
                    reply.recv(child_socket);
                    if (reply.peekstr(0) == "OK") {
                        got_reply = 1;
                        break;
                    }
                }
                if (got_reply == 0) {
                    reply.pushstr("Node " + std::to_string(child_id) + " is not avail");
                }
            } else {
                reply.pushstr("OK");
            }
            reply.send(parent_socket);
        } else if (dest_id == my_id) {
            if (command == "pid") { // проверяем, что с новым процессом всё ок
                reply.pushstr("OK:" + std::to_string(getpid()));
                reply.send(parent_socket);
                reply.clear();
                msg.clear();
                continue;
            } else if (command == "create") {
                if (child_id != -1) { // так как у нас списки, больше одного ребёнка не можем иметь
                    reply.pushstr("Node already has a child!");
                    reply.send(parent_socket);
                    reply.clear();
                    msg.clear();
                } else { // добавляем ребёнка к узлу
                    child_id = stoi(msg.peekstr(2));
                    child_socket.bind(adr(child_id));
                    pid_t pid = fork();
                    if (pid == 0) {
                        execl("./calc", "./calc", std::to_string(child_id).c_str(), NULL);
                        perror("Can't create new process!\n");
                        exit(1);
                    }
                    passing.pushstr("pid");
                    passing.pushstr(std::to_string(child_id));
                    passing.send(child_socket);
                    reply.recv(child_socket);
                    reply.send(parent_socket);     
                }
                continue;
            } else if (command == "kill") { // отсоединяем все дочерние узлы, а затем текущий
                if (child_id != -1) {
                    passing.pushstr("kill");
                    passing.pushstr(std::to_string(child_id));
                    passing.send(child_socket);
                    reply.recv(child_socket);
                    reply.send(parent_socket);
                    child_socket.unbind(adr(child_id));
                } else {
                    reply.pushstr("OK");
                    reply.send(parent_socket);
                }
                parent_socket.disconnect(adr(my_id));
                break;
            } else if (command == "exec") { // исполнение узла
                std::string test_string = msg.peekstr(2);
                std::string pattern_string = msg.peekstr(3);
                std::string message = "OK:" + std::to_string(my_id) + ":";
                int n = test_string.length();
                int m = pattern_string.length();
                for (int i = 0; i <= n - m; ++i) {
                    int j;
                    for (j = 0; j < m; ++j) {
                        if (test_string[i + j] != pattern_string[j]) {
                            break;
                        }
                    }
                    if (j == m) {
                        message += std::to_string(i)+';';
                    }
                }
                reply.pushstr(message);
                reply.send(parent_socket);
            }
        } else if (child_id != -1) { // если сообщение не для данного узла, отправляем его дальше
            msg.send(child_socket);
            reply.recv(child_socket);
            reply.send(parent_socket);
            if (child_id == dest_id && command == "kill") {
                child_socket.unbind(adr(child_id));
                child_id = -1;
            }
        } else {
            reply.pushstr("Error: node is unavailable!\n");
            reply.send(parent_socket);
        }
    }
}