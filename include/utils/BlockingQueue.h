#ifndef MANDEYE_MULTISENSOR_BLOCKINGQUEUE_H
#define MANDEYE_MULTISENSOR_BLOCKINGQUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

namespace utils
{

template <typename T>
class BlockingQueue
{
private:
	std::queue<T> queue;
	std::mutex mutex;
	std::condition_variable signal;
	bool running = true;

public:
	void push(T value) {
		std::lock_guard<std::mutex> lock(mutex);
		queue.push(value);
		signal.notify_one();
	}

	T pop() {
		std::unique_lock<std::mutex> lock(mutex);
		signal.wait(lock, [this] { return !queue.empty() || !running; });
		if (!running)
			return T();
		T value = queue.front();
		queue.pop();
		return value;
	}

	bool empty() {
		std::lock_guard<std::mutex> lock(mutex);
		return queue.empty();
	}

	size_t size() {
		std::lock_guard<std::mutex> lock(mutex);
		return queue.size();
	}

	void dropN(size_t n) {
		std::lock_guard<std::mutex> lock(mutex);
		while (n-- && !queue.empty())
			queue.pop();
	}

	void keepN(size_t n) {
		std::lock_guard<std::mutex> lock(mutex);
		while (queue.size() > n)
			queue.pop();
	}

	void stop() {
		std::lock_guard<std::mutex> lock(mutex);
		running = false;
		signal.notify_all();
	}


};

} // namespace utils

#endif //MANDEYE_MULTISENSOR_BLOCKINGQUEUE_H
