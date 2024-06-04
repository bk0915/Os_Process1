//2-1, 2-2, 2-3����
#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

using namespace std;

// Process Ŭ����: ���μ��� ������ ����
class Process {
public:
    int pid;  // ���μ��� ID
    string type;  // ���μ��� Ÿ��: F (Foreground) �Ǵ� B (Background)
    string command;  // ������ ��ɾ�
    chrono::time_point<chrono::system_clock> wake_time;  // ��� ť�� �ִ� ���μ����� ���� �ð�
    bool operator>(const Process& other) const {
        return wake_time > other.wake_time;
    }
};

class ProcessManager {
private:
    vector<thread> threads;
    vector<Process> process_list;
    priority_queue<Process, vector<Process>, greater<Process>> wait_queue;
    mutex mtx;
    condition_variable cv;
    bool running = true;

    // Section 2-1: Dynamic Queueing
    void shell();  // ��ɾ �а� �����ϴ� �� ���μ���
    void monitor();  // �ý��� ���¸� ����ϴ� ����� ���μ���

    // Section 2-2: Alarm Clock
    void execute_command(Process& process);  // ��ɾ� ����
    vector<string> parse(const string& command);  // ��ɾ ��ū���� �Ľ�

    // ��ɾ� ����
    int gcd(int a, int b);
    int prime_count(int n);
    int sum(int n);
    void echo(const vector<string>& args);
    void dummy();
    void gcd_command(const vector<string>& args);
    void prime_command(const vector<string>& args);
    void sum_command(const vector<string>& args);

public:
    ProcessManager();
    ~ProcessManager();
    void start();  // ���μ��� �Ŵ��� ����
    void stop();  // ���μ��� �Ŵ��� ����
};

// ������: ���μ��� �Ŵ��� �ʱ�ȭ
ProcessManager::ProcessManager() {
    process_list.push_back({ 0, "F", "shell" }); // Foreground �� ���μ���
    process_list.push_back({ 1, "B", "monitor" }); // Background ����� ���μ���
}

// �Ҹ���: ���μ��� �Ŵ��� ����
ProcessManager::~ProcessManager() {
    stop();
}

// Section 2-3: CLI
void ProcessManager::start() {
    threads.push_back(thread(&ProcessManager::shell, this));
    threads.push_back(thread(&ProcessManager::monitor, this));
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }
}

// ���μ��� �Ŵ��� ����
void ProcessManager::stop() {
    {
        unique_lock<mutex> lock(mtx);
        running = false;
    }
    cv.notify_all();
}

// �� ���μ���: ���Ͽ��� ��ɾ �а� ����
void ProcessManager::shell() {
    ifstream infile("commands.txt"); // �� ������ ���� ���ϰ� ������ ���丮�� �־�� �մϴ�
    if (!infile) {
        cerr << "Error: Could not open commands.txt" << endl;

        // ���� �۾� ���丮 ���
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            cerr << "Current working directory: " << cwd << endl;
        }
        else {
            cerr << "Error getting current working directory" << endl;
        }

        return;
    }

    string line;
    int pid_counter = 2;

    while (running) {
        if (getline(infile, line)) {
            stringstream ss(line);
            string command;
            while (getline(ss, command, ';')) {
                unique_lock<mutex> lock(mtx);
                process_list.push_back({ pid_counter++, "F", command });
                cv.notify_all();
                this_thread::sleep_for(chrono::seconds(5)); // ����� �Է��� ����ϱ� ���� ����
            }
        }
        else {
            this_thread::sleep_for(chrono::seconds(5)); // ��ɾ ������ ����
        }
    }
}

// ����� ���μ���: �ý��� ���� ���
void ProcessManager::monitor() {
    while (running) {
        this_thread::sleep_for(chrono::seconds(5)); // ����� ����

        unique_lock<mutex> lock(mtx);
        cout << "Running: [";
        for (auto& process : process_list) {
            cout << process.pid << process.type << " ";
        }
        cout << "]" << endl;

        cout << "---------------------------" << endl;
        cout << "DQ: ";
        // Dynamic Queue ��� (������)
        for (auto& process : process_list) {
            cout << process.pid << process.type << " ";
        }
        cout << endl;

        cout << "---------------------------" << endl;
        cout << "WQ: ";
        // Wait Queue ��� (������)
        priority_queue<Process, vector<Process>, greater<Process>> temp_queue = wait_queue;
        while (!temp_queue.empty()) {
            Process p = temp_queue.top();
            temp_queue.pop();
            cout << p.pid << p.type << ":" << chrono::duration_cast<chrono::seconds>(p.wake_time - chrono::system_clock::now()).count() << " ";
        }
        cout << endl << "..." << endl;
    }
}

// ���μ����� ��ɾ� ����
void ProcessManager::execute_command(Process& process) {
    vector<string> args = parse(process.command);

    if (args.empty()) return;

    if (args[0] == "echo") {
        echo(args);
    }
    else if (args[0] == "dummy") {
        dummy();
    }
    else if (args[0] == "gcd") {
        gcd_command(args);
    }
    else if (args[0] == "prime") {
        prime_command(args);
    }
    else if (args[0] == "sum") {
        sum_command(args);
    }
}

// ��ɾ ��ū���� �Ľ�
vector<string> ProcessManager::parse(const string& command) {
    stringstream ss(command);
    string token;
    vector<string> tokens;

    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// �ִ�����(GCD) ���
int ProcessManager::gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

// n ������ �Ҽ� ���� ���
int ProcessManager::prime_count(int n) {
    vector<bool> is_prime(n + 1, true);
    is_prime[0] = is_prime[1] = false;
    for (int i = 2; i * i <= n; ++i) {
        if (is_prime[i]) {
            for (int j = i * i; j <= n; j += i) {
                is_prime[j] = false;
            }
        }
    }
    return static_cast<int>(count(is_prime.begin(), is_prime.end(), true));
}

// 1���� n������ ���� 1000000���� ���� ������ ���
int ProcessManager::sum(int n) {
    return (n * (n + 1) / 2) % 1000000;
}

// echo ��ɾ� ����
void ProcessManager::echo(const vector<string>& args) {
    for (size_t i = 1; i < args.size(); ++i) {
        cout << args[i] << " ";
    }
    cout << endl;
}

// dummy ��ɾ� ���� (�ƹ� �۾��� ���� ����)
void ProcessManager::dummy() {
    // Dummy ���μ����� �ƹ� �͵� ���� ����
}

// GCD ��ɾ� ����
void ProcessManager::gcd_command(const vector<string>& args) {
    if (args.size() < 3) return;
    int a = stoi(args[1]);
    int b = stoi(args[2]);
    cout << "GCD of " << a << " and " << b << " is " << gcd(a, b) << endl;
}

// Prime ��ɾ� ����
void ProcessManager::prime_command(const vector<string>& args) {
    if (args.size() < 2) return;
    int n = stoi(args[1]);
    cout << "Number of primes <= " << n << " is " << prime_count(n) << endl;
}

// Sum ��ɾ� ����
void ProcessManager::sum_command(const vector<string>& args) {
    if (args.size() < 2) return;
    int n = stoi(args[1]);
    cout << "Sum of numbers <= " << n << " % 1000000 is " << sum(n) << endl;
}

// ���� �Լ�: ���μ��� �Ŵ��� ����
int main() {
    ProcessManager pm;
    pm.start();
    return 0;
}
