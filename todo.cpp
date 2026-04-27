#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>
#include <ctime>
#include <algorithm>

namespace Color {
    const std::string RESET   = "";
    const std::string BOLD    = "";
    const std::string RED     = "";
    const std::string GREEN   = "";
    const std::string YELLOW  = "";
    const std::string CYAN    = "";
    const std::string MAGENTA = "";
    const std::string GREY    = "";
}

enum class Priority { HIGH = 1, MEDIUM = 2, LOW = 3 };

std::string priorityToString(Priority p) {
    switch (p) {
        case Priority::HIGH:   return "HIGH";
        case Priority::MEDIUM: return "MED ";
        case Priority::LOW:    return "LOW ";
    }
    return "MED ";
}

std::string priorityColor(Priority p) {
    switch (p) {
        case Priority::HIGH:   return Color::RED;
        case Priority::MEDIUM: return Color::YELLOW;
        case Priority::LOW:    return Color::GREEN;
    }
    return Color::YELLOW;
}

Priority priorityFromString(const std::string& s) {
    if (s == "HIGH") return Priority::HIGH;
    if (s == "LOW")  return Priority::LOW;
    return Priority::MEDIUM;
}

struct Task {
    int         id;
    std::string title;
    bool        completed;
    Priority    priority;
    std::string createdAt;

    std::string serialise() const {
        return std::to_string(id) + "|"
             + title + "|"
             + (completed ? "1" : "0") + "|"
             + priorityToString(priority) + "|"
             + createdAt;
    }

    static bool deserialise(const std::string& line, Task& out) {
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, '|'))
            parts.push_back(token);
        if (parts.size() < 5) return false;
        try {
            out.id        = std::stoi(parts[0]);
            out.title     = parts[1];
            out.completed = (parts[2] == "1");
            out.priority  = priorityFromString(parts[3]);
            out.createdAt = parts[4];
        } catch (...) { return false; }
        return true;
    }
};

class TodoList {
public:
    explicit TodoList(const std::string& filepath) : m_file(filepath), m_nextId(1) {
        load();
    }

    void addTask(const std::string& title, Priority priority) {
        Task t;
        t.id        = m_nextId++;
        t.title     = title;
        t.completed = false;
        t.priority  = priority;
        t.createdAt = currentTimestamp();
        m_tasks.push_back(t);
        save();
        std::cout << "\n  Task #" << t.id << " added successfully.\n";
    }

    bool markCompleted(int id) {
        Task* t = findById(id);
        if (!t) return false;
        if (t->completed) {
            std::cout << "  Task #" << id << " is already completed.\n";
            return true;
        }
        t->completed = true;
        save();
        std::cout << "\n  Task #" << id << " marked as completed.\n";
        return true;
    }

    bool deleteTask(int id) {
        auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
                               [id](const Task& t){ return t.id == id; });
        if (it == m_tasks.end()) return false;
        m_tasks.erase(it);
        save();
        std::cout << "\n  Task #" << id << " deleted.\n";
        return true;
    }

    enum class Filter { ALL, PENDING, COMPLETED };

    void viewTasks(Filter filter = Filter::ALL) const {
        std::vector<const Task*> visible;
        for (const auto& t : m_tasks) {
            if (filter == Filter::PENDING   &&  t.completed) continue;
            if (filter == Filter::COMPLETED && !t.completed) continue;
            visible.push_back(&t);
        }

        printDivider();
        std::string header = "  TO-DO LIST";
        if (filter == Filter::PENDING)   header += "  [ PENDING ]";
        if (filter == Filter::COMPLETED) header += "  [ COMPLETED ]";
        std::cout << header << "\n";
        printDivider();

        if (visible.empty()) {
            std::cout << "  (no tasks to show)\n";
        } else {
            const int W_ID     = 4;
            const int W_STATUS = 12;
            const int W_PRI    = 6;
            const int W_TITLE  = 36;
            const int W_DATE   = 18;

            std::cout << "  " << std::left
                      << std::setw(W_ID)     << "ID"
                      << std::setw(W_STATUS) << "STATUS"
                      << std::setw(W_PRI)    << "PRI"
                      << std::setw(W_TITLE)  << "TITLE"
                      << std::setw(W_DATE)   << "CREATED"
                      << "\n";
            printDivider();

            for (const Task* t : visible) {
                std::string statusStr = t->completed ? "Done   " : "Pending";
                std::string priStr    = priorityToString(t->priority);

                std::cout << "  " << std::left
                          << std::setw(W_ID)     << t->id
                          << std::setw(W_STATUS) << statusStr
                          << std::setw(W_PRI)    << priStr
                          << std::setw(W_TITLE)  << truncate(t->title, W_TITLE - 1)
                          << std::setw(W_DATE)   << t->createdAt
                          << "\n";
            }
        }

        printDivider();
        int total     = static_cast<int>(m_tasks.size());
        int pending   = static_cast<int>(std::count_if(m_tasks.begin(), m_tasks.end(),
                            [](const Task& t){ return !t.completed; }));
        int completed = total - pending;
        std::cout << "  Total: " << total
                  << "  |  Pending: " << pending
                  << "  |  Done: " << completed
                  << "\n\n";
    }

    int size() const { return static_cast<int>(m_tasks.size()); }

private:
    std::string       m_file;
    std::vector<Task> m_tasks;
    int               m_nextId;

    void save() const {
        std::ofstream f(m_file);
        if (!f) return;
        f << m_nextId << "\n";
        for (const auto& t : m_tasks)
            f << t.serialise() << "\n";
    }

    void load() {
        std::ifstream f(m_file);
        if (!f) return;
        std::string line;
        if (std::getline(f, line)) {
            try { m_nextId = std::stoi(line); } catch (...) {}
        }
        while (std::getline(f, line)) {
            Task t;
            if (Task::deserialise(line, t))
                m_tasks.push_back(t);
        }
    }

    Task* findById(int id) {
        for (auto& t : m_tasks)
            if (t.id == id) return &t;
        return nullptr;
    }

    static std::string currentTimestamp() {
        std::time_t now = std::time(nullptr);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::localtime(&now));
        return buf;
    }

    static std::string truncate(const std::string& s, size_t maxLen) {
        if (s.size() <= maxLen) return s;
        return s.substr(0, maxLen - 3) + "...";
    }

    static void printDivider() {
        std::cout << "  " << std::string(78, '-') << "\n";
    }
};

void clearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int readInt(const std::string& prompt) {
    int v;
    while (true) {
        std::cout << prompt;
        if (std::cin >> v) { clearInput(); return v; }
        std::cout << "  Invalid input. Please enter a number.\n";
        clearInput();
    }
}

std::string readLine(const std::string& prompt) {
    std::string line;
    std::cout << prompt;
    std::getline(std::cin, line);
    size_t start = line.find_first_not_of(" \t");
    size_t end   = line.find_last_not_of(" \t");
    return (start == std::string::npos) ? "" : line.substr(start, end - start + 1);
}

Priority readPriority() {
    std::cout << "  Priority - 1) High  2) Medium  3) Low  [default: 2]: ";
    std::string s;
    std::getline(std::cin, s);
    if (s == "1") return Priority::HIGH;
    if (s == "3") return Priority::LOW;
    return Priority::MEDIUM;
}

void printMenu() {
    std::cout << "\n"
              << "  ================================\n"
              << "     TO-DO  LIST  MANAGER\n"
              << "  ================================\n"
              << "  1  Add a new task\n"
              << "  2  View all tasks\n"
              << "  3  View pending tasks\n"
              << "  4  View completed tasks\n"
              << "  5  Mark a task as completed\n"
              << "  6  Delete a task\n"
              << "  0  Exit\n"
              << "\n";
}

int main() {
    const std::string DATA_FILE = "todo_data.txt";
    TodoList list(DATA_FILE);

    std::cout << "\n  Welcome to the Console To-Do List App!\n"
              << "  (Data saved automatically to: " << DATA_FILE << ")\n";

    while (true) {
        printMenu();
        int choice = readInt("  Enter choice: ");

        switch (choice) {
            case 1: {
                std::string title = readLine("  Task title: ");
                if (title.empty()) {
                    std::cout << "  Title cannot be empty.\n";
                    break;
                }
                Priority pri = readPriority();
                list.addTask(title, pri);
                break;
            }
            case 2:
                list.viewTasks(TodoList::Filter::ALL);
                break;
            case 3:
                list.viewTasks(TodoList::Filter::PENDING);
                break;
            case 4:
                list.viewTasks(TodoList::Filter::COMPLETED);
                break;
            case 5: {
                list.viewTasks(TodoList::Filter::PENDING);
                int id = readInt("  Enter task ID to mark as completed: ");
                if (!list.markCompleted(id))
                    std::cout << "  Task #" << id << " not found.\n";
                break;
            }
            case 6: {
                list.viewTasks(TodoList::Filter::ALL);
                int id = readInt("  Enter task ID to delete: ");
                if (!list.deleteTask(id))
                    std::cout << "  Task #" << id << " not found.\n";
                break;
            }
            case 0:
                std::cout << "\n  Goodbye! Your tasks have been saved.\n\n";
                return 0;
            default:
                std::cout << "  Invalid option. Choose 0-6.\n";
        }
    }
}