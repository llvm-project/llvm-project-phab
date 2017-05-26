#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

std::mutex mutex;
std::condition_variable cond;

void *
thread3(void *input)
{
    {
        // Synchronization of multiple threads before the thread3 breakpoints.
        std::unique_lock<std::mutex> lock(mutex);
        cond.notify_all();
    }

    std::unique_lock<std::mutex> lock(mutex); // Set thread3 break point on lock at this line.
    cond.notify_all(); // Set thread3 break point on notify_all at this line.
    return NULL;
}

void *
thread2(void *input)
{
    std::unique_lock<std::mutex> lock(mutex);
    cond.notify_all();
    cond.wait(lock);
    return NULL;
}

void *
thread1(void *input)
{
    std::thread thread_2(thread2, nullptr);
    thread_2.join();

    return NULL;
}

int main()
{
    std::unique_lock<std::mutex> lock(mutex);

    std::thread thread_1(thread1, nullptr);
    cond.wait(lock);

    std::vector<std::thread> thread_3s;
    for (int i = 0; i < 10; i++) {
      thread_3s.push_back(std::thread(thread3, nullptr));
    }

    cond.wait(lock); // Set break for first wait in thread3 at this line.

    cond.wait(lock);

    lock.unlock();

    thread_1.join();
    for (auto &t : thread_3s){
        t.join();
    }

    return 0;
}
