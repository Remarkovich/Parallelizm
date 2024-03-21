#include <iostream>
#include <thread>
#include <list>
#include <mutex>
#include <queue>
#include <future>
#include <functional>
#include <condition_variable>
#include <random>
#include <cmath> // Для std::sin
#include <unordered_map>

template <typename T>
class TaskServer
{
public:
    void start()
    {
        stop_flag_ = false;
        server_thread_ = std::thread(&TaskServer::server_thread, this);
    }
    void stop()
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stop_flag_ = true;
            cv_.notify_one(); // Уведомляем поток сервера, чтобы он проснулся
        }
        server_thread_.join();
    }
    size_t add_task(std::function<T()> task)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.push({ next_id_++, std::async(std::launch::deferred, task) });
        cv_.notify_one();
        return tasks_.back().first;
    }
    T request_result(size_t id)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this, id]() { return results_.find(id) != results_.end(); });
        T result = results_[id];
        results_.erase(id); // удаляется результат из контейнера после того как его вытащили
        return result;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread server_thread_;
    bool stop_flag_ = false;
    size_t next_id_ = 1;
    //очередь задач, первая переменная в паре это индификатор задачи, вторая объект результата задачи 
    std::queue<std::pair<size_t, std::future<T>>> tasks_;
    std::unordered_map<size_t, T> results_;

    void server_thread() {
        while (true) {
            std::unique_lock<std::mutex> lock(mutex_); // блокировщик
            cv_.wait(lock, [this]() { return !tasks_.empty() || stop_flag_; }); // ожидает сообщения держа блокировщик открытым
            if (tasks_.empty() && stop_flag_) {
                break; // Выход из цикла, если нет задач и флаг stop_flag_ установлен
            }
            if (!tasks_.empty()) {
                auto task = std::move(tasks_.front()); // извлекаем задачу из очереди
                tasks_.pop(); // удаляется задача из очереди
                results_[task.first] = task.second.get(); //ответ для задачи которую вытащили записывается в results_ по её ключу
                cv_.notify_one(); // оповещеает один поток о том что задача готова
            }
        }
    }
};

template <typename T>
class TaskClient {
public:
    void run_client(TaskServer<T>& server, std::function<T()> task_generator) {
        int id = server.add_task(task_generator);
        task_ids_.push_back(id);
    }

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
    // Генерация случайного числа в диапазоне от -? до ?
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
    TaskServer<double> server; // Уточняем тип double
    server.start();

    TaskClient<double> client1;
    TaskClient<double> client2;
    TaskClient<double> client3;

    for (int i = 0; i < 10; ++i) {
        client1.run_client(server, fun_random_sin<double>);
        client2.run_client(server, fun_random_sqrt<double>);
        client3.run_client(server, fun_random_pow<double>);
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
