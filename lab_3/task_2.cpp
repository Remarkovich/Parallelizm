#include <iostream>
#include <thread>
#include <list>
#include <mutex>
#include <queue>
#include <future>
#include <functional>
#include <condition_variable>
#include <random>
#include <cmath> 
#include <unordered_map>

// Класс сервера задач
template <typename T>
class TaskServer
{
public:
    // Запуск сервера задач
    void start()
    {
        stop_flag_ = false;
        server_thread_ = std::thread(&TaskServer::server_thread, this);
    }

    // Остановка сервера задач
    void stop()
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stop_flag_ = true;
            cv_.notify_one(); //здесь мы уведомляем поток, что сервер должен быть остановлен
        }
        server_thread_.join();
    }

    // Добавление задачи в очередь на выполнение
    size_t add_task(std::function<T()> task)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.push({ ++next_id_, std::async(std::launch::deferred, task) }); 
        cv_.notify_one(); // уведомляем поток сервера
        return tasks_.back().first;
    }

    // Запрос результата выполнения задачи по её идентификатору
    T request_result(size_t id)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this, id]() {return results_.find(id) != results_.end(); }); // ждём ответа от потока сервера
        T result = results_[id];
        results_.erase(id); 
        return result;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread server_thread_;
    bool stop_flag_ = false;
    size_t next_id_ = 1;
    std::queue<std::pair<size_t, std::future<T>>> tasks_;
    std::unordered_map<size_t, T> results_;

    // Основной поток сервера, который обрабатывает задачи из очереди
    void server_thread() {
        while (true) {
            std::unique_lock<std::mutex> lock(mutex_); 
            cv_.wait(lock, [this]() { return !tasks_.empty() || stop_flag_;  }); // ожидает сообщения 
            if (tasks_.empty() && stop_flag_) {
                break;
            }
            if (!tasks_.empty()) {
                auto task = std::move(tasks_.front());
                tasks_.pop(); 
                results_[task.first] = task.second.get();
                cv_.notify_one(); // оповещеает клиентский поток о добавление задачи в контейнер results
            }
        }
    }
};

// Класс клиента задач
template <typename T>
class TaskClient {
public:
    // Запуск клиента задач с заданной задачей-генератором
    void run_client(TaskServer<T>& server, std::function<T()> task_generator) {
        int id = server.add_task(task_generator);
        task_ids_.push_back(id);
    }

    // Получение результатов выполнения задач клиента
    std::list<T> client_to_result(TaskServer<T>& server) {
        std::list<T> results;
        for (int id : task_ids_) {
            T result = server.request_result(id);
            results.push_back(result);
        }
        return results;
    }

private:
    std::vector<int> task_ids_;
};

template<typename T>
T fun_random_sin() {
    
    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(-3.14159, 3.14159);
    T x = distribution(generator);
    return std::sin(x);
}

template<typename T>
T fun_random_sqrt() {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(1.0, 10.0);
    T value = distribution(generator);
    return std::sqrt(value);
}

template<typename T>
T fun_random_pow() {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<T> distribution(1.0, 10.0);
    T value = distribution(generator);
    return std::pow(value, 2.0);
}

int main() {
    TaskServer<double> server; 
    server.start();

    TaskClient<double> client1;
    TaskClient<double> client2;
    TaskClient<double> client3;

    for (int i = 0; i < 10; ++i) {
        client1.run_client(server, fun_random_sin<double>);
        client2.run_client(server, fun_random_sqrt<double>);
        client3.run_client(server, fun_random_pow<double>);

        /*std::this_thread::sleep_for(std::chrono::seconds(2));*/
    }

    std::list<double> t1 = client1.client_to_result(server);
    std::list<double> t2 = client2.client_to_result(server);
    std::list<double> t3 = client3.client_to_result(server);

    std::cout << "t1: ";
    for (double n : t1)
        std::cout << n << ", ";
    std::cout << "\n";

    std::cout << "t2: ";
    for (double n : t2)
        std::cout << n << ", ";
    std::cout << "\n";

    std::cout << "t3: ";
    for (double n : t3)
        std::cout << n << ", ";
    std::cout << "\n";
    
    server.stop();

    return 0;
}
