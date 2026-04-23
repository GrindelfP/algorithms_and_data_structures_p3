#include <iostream>
#include <numeric>
#include <utility>
#include <stdexcept>
#include <initializer_list>

template<typename T>
class LList {
private:
    struct Node {
        T data;
        Node *next;

        Node() : next(nullptr) {
        }

        explicit Node(const T &val) : data(val), next(nullptr) {
        }
    };

    Node *p;
    size_t list_size;

    void init() {
        p = new Node();
        p->next = p;
        list_size = 0;
    }

public:
    class ListIterator {
    private:
        Node *current;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using reference = T &;

        explicit ListIterator(Node *node) : current(node) {
        }

        T &operator*() { return current->data; }

        ListIterator &operator++() {
            current = current->next;
            return *this;
        }

        ListIterator operator++(int) {
            ListIterator temp = *this;
            current = current->next;
            return temp;
        }

        bool operator!=(const ListIterator &other) const { return current != other.current; }
        bool operator==(const ListIterator &other) const { return current == other.current; }
    };

    // Constructor
    LList() {
        init();
    }

    // Initializer list constructor
    LList(std::initializer_list<T> init_list) {
        init();
        for (const T &val: init_list) {
            push_back(val);
        }
    }

    // Destructor
    ~LList() {
        clear();
        delete p;
    }

    // Copy Constructor
    LList(const LList &other) {
        init();
        Node *curr = other.p->next->next;
        for (size_t i = 0; i < other.list_size; ++i) {
            push_back(curr->data);
            curr = curr->next;
        }
    }

    // Copy Assignment Operator
    LList &operator=(const LList &other) {
        if (this != &other) {
            LList temp(other);
            std::swap(p, temp.p);
            std::swap(list_size, temp.list_size);
        }
        return *this;
    }

    // Move Constructor
    LList(LList &&other) noexcept : p(other.p), list_size(other.list_size) {
        other.init();
    }

    // Move Assignment Operator
    LList &operator=(LList &&other) noexcept {
        if (this != &other) {
            clear();
            delete p;

            p = other.p;
            list_size = other.list_size;

            other.init();
        }
        return *this;
    }

    void push_back(const T &value) {
        Node *sentinel = p->next;
        Node *new_node = new Node(value);
        new_node->next = sentinel;
        p->next = new_node;
        p = new_node;
        list_size++;
    }

    void push_front(const T &value) {
        Node *sentinel = p->next;
        Node *new_node = new Node(value);
        new_node->next = sentinel->next;
        sentinel->next = new_node;
        if (list_size == 0) {
            p = new_node;
        }
        list_size++;
    }

    void insert(size_t index, const T &value) {
        if (index > list_size) throw std::out_of_range("Index out of bounds");
        if (index == 0) {
            push_front(value);
            return;
        }
        if (index == list_size) {
            push_back(value);
            return;
        }

        Node *curr = p->next;
        for (size_t i = 0; i < index; ++i) {
            curr = curr->next;
        }

        Node *new_node = new Node(value);
        new_node->next = curr->next;
        curr->next = new_node;
        list_size++;
    }

    void pop_back() {
        if (empty()) return;
        Node *sentinel = p->next;
        Node *curr = sentinel;

        while (curr->next != p) {
            curr = curr->next;
        }

        curr->next = p->next;
        delete p;
        p = curr;
        list_size--;
    }

    void pop_front() {
        if (empty()) return;
        Node *sentinel = p->next;
        Node *first = sentinel->next;
        sentinel->next = first->next;

        if (list_size == 1) {
            p = sentinel;
        }

        delete first;
        list_size--;
    }

    void remove_at(size_t index) {
        if (index >= list_size) throw std::out_of_range("Index out of bounds");
        if (index == 0) {
            pop_front();
            return;
        }
        if (index == list_size - 1) {
            pop_back();
            return;
        }

        Node *curr = p->next;
        for (size_t i = 0; i < index; ++i) {
            curr = curr->next;
        }

        Node *to_delete = curr->next;
        curr->next = to_delete->next;
        delete to_delete;
        list_size--;
    }

    T &operator[](size_t index) {
        if (index >= list_size) throw std::out_of_range("Index out of bounds");
        Node *curr = p->next->next;
        for (size_t i = 0; i < index; ++i) {
            curr = curr->next;
        }
        return curr->data;
    }

    const T &operator[](size_t index) const {
        if (index >= list_size) throw std::out_of_range("Index out of bounds");
        Node *curr = p->next->next;
        for (size_t i = 0; i < index; ++i) {
            curr = curr->next;
        }
        return curr->data;
    }

    size_t size() const { return list_size; }

    bool empty() const { return list_size == 0; }

    void clear() {
        while (!empty()) {
            pop_front();
        }
    }

    const T &front() const {
        if (empty()) throw std::out_of_range("List is empty");
        return p->next->next->data;
    }

    const T &back() const {
        if (empty()) throw std::out_of_range("List is empty");
        return p->data;
    }

    ListIterator begin() { return ListIterator(p->next->next); }
    ListIterator end() { return ListIterator(p->next); }

    ListIterator begin() const { return ListIterator(p->next->next); }
    ListIterator end() const { return ListIterator(p->next); }
};

// Test output formatting function
template<typename T>
void print_lst(const LList<T> &l) {
    if (l.empty()) {
        std::cout << std::endl;
        return;
    }
    for (auto it = l.begin(); it != l.end(); ++it) {
        std::cout << *it;
        auto next_it = it;
        ++next_it;
        if (next_it != l.end()) {
            std::cout << "->";
        }
    }
    std::cout << std::endl;
}

int main() {
    // 1. Iterator and STL algorithm test
    LList<int> l = {1, 5, 2, 7, 8};
    int s = 0;
    for (auto i: l) s += i * 10;
    auto lambda = [](int a, int b) { return a + b * 10; };
    std::cout << s << '\t' << std::accumulate(l.begin(), l.end(), 0, lambda) << '\n';

    // 2. Main functionality tests
    LList<char> lst;
    std::cout << std::boolalpha << lst.empty() << std::endl;

    for (int i = 0; i < 5; i++) lst.push_back(static_cast<char>('a' + i));
    print_lst(lst);

    for (int i = 0; i < 5; i++) lst.insert(0, static_cast<char>('z' - i));
    print_lst(lst);

    for (size_t i = 0; i != lst.size(); i++) lst[i] = static_cast<char>('a' + i);
    print_lst(lst);

    lst.pop_back();
    lst.pop_front();
    print_lst(lst);

    lst.remove_at(5);
    lst.insert(3, 'o');
    print_lst(lst);

    lst.clear();
    lst.push_back('q');
    lst.push_back('w');
    std::cout << lst.size() << " " << std::boolalpha << lst.empty() << std::endl;

    // 3. Rule of Five tests

    // Copy constructor test
    LList<char> a;
    for (char c = 'a'; c <= 'e'; c++) a.push_back(c);
    LList<char> b = a;
    b[0] = 'Z';
    print_lst(a);
    print_lst(b);

    // Copy assignment test
    LList<char> c_lst;
    for (char c = 'a'; c <= 'e'; c++) c_lst.push_back(c);
    LList<char> d = c_lst;
    d[1] = 'X';
    print_lst(c_lst);
    print_lst(d);

    // Move constructor test
    LList<char> e;
    for (char c = 'a'; c <= 'e'; c++) e.push_back(c);
    LList<char> f = std::move(e);
    print_lst(f);
    std::cout << "size a = " << e.size() << std::endl;

    // Extra boundary checks
    LList<int> num_lst;
    num_lst.push_back(1);
    num_lst.push_back(2);
    num_lst.push_back(3);
    num_lst.insert(0, 100);
    num_lst.insert(num_lst.size(), 200);
    print_lst(num_lst);

    return 0;
}
