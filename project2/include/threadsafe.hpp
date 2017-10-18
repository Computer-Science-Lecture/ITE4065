#ifndef COUNTER_HPP
#define COUNTER_HPP

#include <vector>
#include <list>

#include <mutex>
#include <shared_mutex>

namespace thread {
    namespace safe {
        template <typename T>
        class Counter {
        public:
            Counter(std::mutex& global) : global(global) { };

            T get(bool g=true) const {
                if (g) global.lock();
                std::shared_lock<std::shared_timed_mutex> lock(mutex);
                if (g) global.unlock();
                return value;
            }

            T add(T v, bool g=true) {
                if (g) global.lock();
                std::unique_lock<std::shared_timed_mutex> lock(mutex);
                if (g) global.unlock();
                value += v;
                return value;
            }

            void reset(bool g=true) {
                if (g) global.lock();
                std::unique_lock<std::shared_timed_mutex> lock(mutex);
                if (g) global.unlock();
                value = 0;
            }

        private:
            mutable std::shared_timed_mutex mutex;
            std::mutex& global;
            T value = 0;
        };

        template <typename T>
        class Container {
        public:
            Container(size_t size, std::mutex& global)
            : global(global), read_wait(size), write_wait(size) {
                while (size--) {
                    elements.push_back(new Counter<T>(global));
                }
            };

            ~Container() {
                for (auto& e : elements) {
                    delete e;
                }
            }

            T read(size_t index, bool g=true) {
                if (!assert_index(index)) throw std::out_of_range("index out of range");
                read_wait.push_back(index);
                T value = elements[index]->get(g);
                read_wait.remove(index);
                return value;
            }

            T write(size_t index, T v, bool g=true) {
                if (!assert_index(index)) throw std::out_of_range("index out of range");
                write_wait.push_back(index);
                T value = elements[index]->add(v, g);
                write_wait.remove(index);
                return value;
            }

        private:
            std::mutex& global;
            std::vector<Counter<T> *> elements;
            std::list<size_t> read_wait;
            std::list<size_t> write_wait;

            bool assert_index(size_t index) {
                return 0 <= index && index < elements.size();
            }
        };
    }
}

#endif