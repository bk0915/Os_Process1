//2-1, 2-2, 2-3구현
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

// Process 클래스: 프로세스 정보를 저장
class Process {
public:
    int pid;  // 프로세스 ID
    string type;  // 프로세스 타입: F (Foreground) 또는 B (Background)
    string command;  // 실행할 명령어
    chrono::time_point<chrono::system_clock> wake_time;  // 대기 큐에 있는 프로세스의 깨울 시간
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
    void shell();  // 명령어를 읽고 실행하는 쉘 프로세스
    void monitor();  // 시스템 상태를 출력하는 모니터 프로세스

    // Section 2-2: Alarm Clock
    void execute_command(Process& process);  // 명령어 실행
    vector<string> parse(const string& command);  // 명령어를 토큰으로 파싱

    // 명령어 구현
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
    void start();  // 프로세스 매니저 시작
    void stop();  // 프로세스 매니저 중지
};

// 생성자: 프로세스 매니저 초기화
ProcessManager::ProcessManager() {
    process_list.push_back({ 0, "F", "shell" }); // Foreground 쉘 프로세스
    process_list.push_back({ 1, "B", "monitor" }); // Background 모니터 프로세스
}

// 소멸자: 프로세스 매니저 중지
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

// 프로세스 매니저 중지
void ProcessManager::stop() {
    {
        unique_lock<mutex> lock(mtx);
        running = false;
    }
    cv.notify_all();
}

// 쉘 프로세스: 파일에서 명령어를 읽고 실행
void ProcessManager::shell() {
    ifstream infile("commands.txt"); // 이 파일이 실행 파일과 동일한 디렉토리에 있어야 합니다
    if (!infile) {
        cerr << "Error: Could not open commands.txt" << endl;

        // 현재 작업 디렉토리 출력
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
                this_thread::sleep_for(chrono::seconds(5)); // 사용자 입력을 모방하기 위해 슬립
            }
        }
        else {
            this_thread::sleep_for(chrono::seconds(5)); // 명령어가 없으면 슬립
        }
    }
}

// 모니터 프로세스: 시스템 상태 출력
void ProcessManager::monitor() {
    while (running) {
        this_thread::sleep_for(chrono::seconds(5)); // 모니터 간격

        unique_lock<mutex> lock(mtx);
        cout << "Running: [";
        for (auto& process : process_list) {
            cout << process.pid << process.type << " ";
        }
        cout << "]" << endl;

        cout << "---------------------------" << endl;
        cout << "DQ: ";
        // Dynamic Queue 출력 (간단히)
        for (auto& process : process_list) {
            cout << process.pid << process.type << " ";
        }
        cout << endl;

        cout << "---------------------------" << endl;
        cout << "WQ: ";
        // Wait Queue 출력 (간단히)
        priority_queue<Process, vector<Process>, greater<Process>> temp_queue = wait_queue;
        while (!temp_queue.empty()) {
            Process p = temp_queue.top();
            temp_queue.pop();
            cout << p.pid << p.type << ":" << chrono::duration_cast<chrono::seconds>(p.wake_time - chrono::system_clock::now()).count() << " ";
        }
        cout << endl << "..." << endl;
    }
}

// 프로세스의 명령어 실행
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

// 명령어를 토큰으로 파싱
vector<string> ProcessManager::parse(const string& command) {
    stringstream ss(command);
    string token;
    vector<string> tokens;

    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// 최대공약수(GCD) 계산
int ProcessManager::gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

// n 이하의 소수 개수 계산
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

// 1부터 n까지의 합을 1000000으로 나눈 나머지 계산
int ProcessManager::sum(int n) {
    return (n * (n + 1) / 2) % 1000000;
}

// echo 명령어 구현
void ProcessManager::echo(const vector<string>& args) {
    for (size_t i = 1; i < args.size(); ++i) {
        cout << args[i] << " ";
    }
    cout << endl;
}

// dummy 명령어 구현 (아무 작업도 하지 않음)
void ProcessManager::dummy() {
    // Dummy 프로세스는 아무 것도 하지 않음
}

// GCD 명령어 구현
void ProcessManager::gcd_command(const vector<string>& args) {
    if (args.size() < 3) return;
    int a = stoi(args[1]);
    int b = stoi(args[2]);
    cout << "GCD of " << a << " and " << b << " is " << gcd(a, b) << endl;
}

// Prime 명령어 구현
void ProcessManager::prime_command(const vector<string>& args) {
    if (args.size() < 2) return;
    int n = stoi(args[1]);
    cout << "Number of primes <= " << n << " is " << prime_count(n) << endl;
}

// Sum 명령어 구현
void ProcessManager::sum_command(const vector<string>& args) {
    if (args.size() < 2) return;
    int n = stoi(args[1]);
    cout << "Sum of numbers <= " << n << " % 1000000 is " << sum(n) << endl;
}

// 메인 함수: 프로세스 매니저 시작
int main() {
    ProcessManager pm;
    pm.start();
    return 0;
}
