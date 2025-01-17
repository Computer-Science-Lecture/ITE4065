#include <iostream>
#include <future>
#include <queue>
#include <chrono>
#include <thread>

#include <argparser.hpp>
#include <snapshot.hpp>
#include <register.hpp>
#include <pool.hpp>
#include <random.hpp>

int main(int argc, char * argv[]) {
    std::ios::sync_with_stdio(false);
    arg::Parser parser;

    parser.argument("N", "thread count");
    parser.argument("T", "set time");

    parser.parse(argc, argv);

    size_t n = parser.get<size_t>("N");
    size_t t = parser.get<size_t>("T", 60);

    // main thread scope
    {
        atomic::Snapshot<int> snapshot(n);		// snapshot instance
        util::Random<int> random;				// util::Random generator
        thread::Pool pool(n);					// thread::Pool for multi thread
        std::queue<std::future<void>> tasks;	// thread::Pool tasks
        
        // time guard to run only the set time
        std::thread time_guard([&pool, &t]() {
            std::this_thread::sleep_for(std::chrono::seconds(t));
            pool.terminate();
        });

        // push tasks to thread::Pool until set time
        while (!pool.is_stop()) {
            tasks.emplace(pool.push([&snapshot, v=random.next()](size_t tid) {
                snapshot.update(tid, v);
            }));
        }
        
        // print update count and release time_guard
        std::cout << "update : " << tasks.size() << '\n';
        time_guard.join();

        // release pending tasks
        while (!tasks.empty()) {
            tasks.front().get();
            tasks.pop();
        }
    }
    return 0;
}