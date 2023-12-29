#include <iostream>
#include <vector>
#include <zmq.hpp>
#include <zmq_addon.hpp>

#include "topology.hpp"

std::string adr(int id) {
    return "tcp://127.0.0.1:" + std::to_string(50000 + id);
}

int main() {
    Topology lists; // список списков всех "вычислительных" узлов
    std::vector<zmq::socket_t> heads; // сокеты дочерних узлов (головы списков)
    zmq::context_t context(1); // контекст для сокетов
    std::string command;
    while (1) {
        std::cout << "> ";
        std::cin >> command;
        zmq::multipart_t msg;
        if (command == "create") { // создание нового узла
            int node_id, parent_id;
            std::cin >> node_id >> parent_id;
            if (lists.find(node_id) != -1) { // проверка на существование узла с таким id
                std::cout << "Error: Already exists" << std::endl;
                continue;
            } else if (parent_id == -1) { // создание нового ответвления от управляющего узла
                pid_t pid = fork();
                if (pid == 0) {
                    execl("./calc", "./calc", std::to_string(node_id).c_str(), NULL);
                    perror("Can't execute new process!\n");
                    exit(1);
                }
                heads.emplace_back(context, ZMQ_REQ);
                heads[heads.size()-1].bind(adr(node_id));
                msg.pushstr("pid");
                msg.pushstr(std::to_string(node_id));
                msg.send(heads[heads.size()-1]);
                msg.clear();
                msg.recv(heads[heads.size()-1]);
                std::cout << msg.peekstr(0) << std::endl;
                lists.insert(node_id, parent_id);
                continue;
            } else if (lists.find(parent_id) == -1) { // проверка на наличие узла-родителя в листе
                std::cout << "Error: Parent not found" << std::endl;
                continue;
            } else { // добавляем узел ребёнком к указанному
                int list_num = lists.find(parent_id);
                msg.pushstr(std::to_string(node_id));
                msg.pushstr("create");
                msg.pushstr(std::to_string(parent_id));
                msg.send(heads[list_num]);
                msg.clear();
                msg.recv(heads[list_num]);
                std::string res = msg.peekstr(0);
                std::cout << res << std::endl;
                if (res[0]!='N') lists.insert(node_id, parent_id);
                continue;
            }
        } else if (command == "kill") { // удаление узла
            int id;
            std::cin >> id;
            int list_num = lists.find(id); 
            if (list_num == -1) { // проверка, существует ли узел
                std::cout << "Error: incorrect node id!\n";
            } else { // удаление узла
                bool is_first = (lists.id_for_pos(list_num) == id);
                msg.pushstr("kill");
                msg.pushstr(std::to_string(id));
                msg.send(heads[list_num]);
                msg.clear();
                msg.recv(heads[list_num]);
                std::cout << msg.peekstr(0) << "\n";
                lists.erase(id);
                if (is_first) { // если узел является головой списка, удаляем его из соответствующего вектора
                    heads[list_num].unbind(adr(id));
                    heads.erase(heads.begin() + list_num);
                }
            }
            continue;
        } else if (command == "exec") { // исполнение узла
            int dest_id;
            std::cin >> dest_id;
            int list_num = lists.find(dest_id);
            if (list_num == -1) {
                std::cout << "Error: id " << dest_id << " not found" << std::endl;
                continue;
            }
            std::string test_string, pattern_string;
            std::cout << "> ";
            std::cin >> test_string;
            std::cout << "> ";
            std::cin >> pattern_string;
            msg.pushstr(pattern_string);
            msg.pushstr(test_string);
            msg.pushstr("exec");
            msg.pushstr(std::to_string(dest_id));
            msg.send(heads[list_num]);
            msg.clear();
            msg.recv(heads[list_num]);
            std::cout << msg.peekstr(0) << std::endl;
        } else if(command == "heartbit") { // проверка узлов на отзывчивость
            int TIME;
            std::cin>>TIME;
            for (int i = 0; i < heads.size(); ++i) { // отправляем в каждый список по команде с харбитом
                int head_id = lists.id_for_pos(i);
                msg.pushstr(std::to_string(TIME));
                msg.pushstr("heartbit");
                msg.pushstr(std::to_string(head_id));
                msg.send(heads[i]);
                msg.clear();
                msg.recv(heads[i]);
                std::cout << msg.peekstr(0) << std::endl;
            }
            continue;
        } else if (command == "exit") { // выход из программы
            for (size_t i = 0; i < heads.size(); ++i) {
                int first_node_id = lists.id_for_pos(i);
                msg.pushstr("kill");
                msg.pushstr(std::to_string(first_node_id));
                msg.send(heads[i]); // отправляем всем головам списков удаление
                msg.clear();
                msg.recv(heads[i]);
                std::string res = msg.peekstr(0);
                if (res != "OK") {
                    std::cout << res << "\n";
                } else {
                    heads[i].unbind(adr(first_node_id));
                }
            }
            exit(0);
        } else {
            std::cout << "Incorrect command!" << std::endl;
        }
    }
}   