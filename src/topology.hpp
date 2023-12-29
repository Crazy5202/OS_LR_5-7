#include <list>

class Topology {
private:
    std::list<std::list<int>> container;
public:
    // Добавление нового узла
    void insert(int id, int parent_id) {
        if (parent_id == -1) {
            std::list<int> new_list;
            new_list.push_back(id);
            container.push_back(new_list);
        }
        else {
            int list_id = find(parent_id);
            auto it1 = container.begin();
            std::advance(it1, list_id);
            for (auto it2 = it1->begin(); it2 != it1->end(); ++it2) {
                if (*it2 == parent_id) {
                    it1->insert(++it2, id);
                    return;
                }
            }
        }
    }

    // Поиск узла с заданным id в списке списков
    int find(int id) {
        int counter = 0;
        for (auto it1 = container.begin(); it1 != container.end(); ++it1) {
            for (auto it2 = it1->begin(); it2 != it1->end(); ++it2) {
                if (*it2 == id) {
                    return counter;
                }
            }
            ++counter;
        }
        return -1;
    }

    // Удаление узла с указанным id
    void erase(int id) {
        int list_id = find(id);
        auto it1 = container.begin();
        std::advance(it1, list_id); // Изменяет переданный итератор
        for (auto it2 = it1->begin(); it2 != it1->end(); ++it2) {
            if (*it2 == id) {
                it1->erase(it2, it1->end());
                if (it1->empty()) {
                    container.erase(it1);
                }
                return;
            }
        }
    }
    // Получение первого id узла в контейнере
    int id_for_pos(int pos) {
        auto it1 = container.begin();
        std::advance(it1, pos);
        if (it1->begin() == it1->end()) {
            return -1;
        }
        return *(it1->begin());
    }
};