#pragma once

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <vector>
#include <iostream>
#include <string>
#include <queue>
#include <unordered_map>

// work with threads
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#include "timer.h"

namespace MT 
{
	typedef unsigned long long int task_id;

	class ThreadPool;

	// abstract task class
	class Task 
	{
		friend class ThreadPool;
	protected:
		enum class TaskStatus
		{
			awaiting,
			completed,
			error
		};

		MT::Task::TaskStatus status;
		std::string description;  // text description of the task (needed for beautiful logging)
		MT::task_id id; // unique task ID
		MT::ThreadPool* thread_pool;

		// thread-running method
		void one_thread_pre_method()
		{
			one_thread_method();
			status = MT::Task::TaskStatus::completed;
		}

	public:

		Task(const std::string& _description)
		{
			description = _description;
			id = 0;
			status = MT::Task::TaskStatus::awaiting;
			thread_pool = nullptr;
		}

		// method for signaling the pool from the current task
		void send_signal(); //need to be defined in .cpp because of cross- reference with ThreadPool

		// abstract method that must be implemented by the user,
		// the body of this function must contain the path for solving the current task
		void virtual one_thread_method() = 0;


	};

	// simple wrapper over std::thread for keeping track
	// state of each thread
	struct Thread 
	{
		std::thread _thread;
		std::atomic<bool> is_working;
	};

	class ThreadPool 
	{

		friend void MT::Task::send_signal();

	public:
		ThreadPool(int count_of_threads);

		~ThreadPool();

		//std::shared_ptr<BWTTask>
		// template function for adding a task to the queue
		template <typename TaskChild>
		MT::task_id add_task(std::shared_ptr <TaskChild>& task)
		{
			std::lock_guard<std::mutex> lock(task_queue_mutex);
			task_queue.push(task); //std::make_shared<TaskChild>(task));
			// assign a unique id to a new task
			// the minimum value of id is 1
			task_queue.back()->id = ++last_task_id;
			// associate a task with the current pool
			task_queue.back()->thread_pool = this;
			tasks_access.notify_one();
			return last_task_id;
		}

		size_t task_queue_size()
		{
			std::lock_guard<std::mutex> lock(task_queue_mutex);
			return task_queue.size();
		}

		// waiting for the current task queue to be completely processed or suspended,
		// returns the id of the task that first signaled and 0 otherwise
		MT::task_id wait_signal();

		// wait for the current task queue to be fully processed,
		// ignoring any pause signals
		void wait();

		// pause processing
		void stop();

		// resumption of processing
		void start();

		// get result by id
		template <typename TaskChild>
		std::shared_ptr<TaskChild> get_result(MT::task_id id) const 
		{
			auto elem = completed_tasks.find(id);
			if (elem != completed_tasks.end())
				return std::reinterpret_pointer_cast<TaskChild>(elem->second);
			else
				return nullptr;
		}

		// get result by id
		template <typename TaskChild>
		std::shared_ptr<TaskChild> pop_result(MT::task_id id)
		{
			auto elem = completed_tasks[id]; //TODO check whether it is safe to erase elements this way.
			//auto elem = completed_tasks.find(id);
			completed_tasks.erase(id);
			return std::reinterpret_pointer_cast<TaskChild>(elem);
		}

		// cleaning completed tasks
		void clear_completed();

		template <typename TaskChild>
		void move_completed(std::vector<std::shared_ptr<TaskChild>>& out)
		{
			{
				std::lock_guard lock(completed_tasks_mutex);

				for (auto iter = completed_tasks.begin(); iter != completed_tasks.end(); ++iter)
				{
					out.push_back(std::reinterpret_pointer_cast<TaskChild>(iter->second));
				}
			}

			clear_completed();
		}

		// setting the logging flag
		void set_logger_flag(bool flag);

		size_t tasks_completed() { return completed_tasks.size(); };

		// return true if at least one of pool threads is working on task at the moment
		bool is_active() const
		{
			if (paused) return false;

			for (const auto& thread : threads)
				if (thread->is_working)
					return true;
			return false;
		}

	private:
		// mutexes blocking queues for thread-safe access
		std::mutex task_queue_mutex;
		std::mutex completed_tasks_mutex;
		std::mutex signal_queue_mutex;

		// mutex blocking serial output logger
		std::mutex logger_mutex;

		// mutex blocking functions waiting for results (wait* methods)
		std::mutex wait_mutex;

		std::condition_variable tasks_access;
		std::condition_variable wait_access;

		// set of available threads
		std::vector<MT::Thread*> threads;

		// task queue
		std::queue<std::shared_ptr<Task>> task_queue;
		MT::task_id last_task_id;

		// array of completed tasks in the form of a hash table
		std::unordered_map<MT::task_id, std::shared_ptr<Task>> completed_tasks;
		unsigned long long completed_task_count;

		std::queue<task_id> signal_queue;

		// pool stop flag
		std::atomic<bool> stopped;
		// pause flag
		std::atomic<bool> paused;
		std::atomic<bool> ignore_signals;
		// flag to enable logging
		std::atomic<bool> logger_flag;

		Timer timer;

		// main function that initializes each thread
		void run(MT::Thread* _thread);

		// pause processing with signal emission
		void receive_signal(MT::task_id id);

		// permission to start the next thread
		bool run_allowed() const;

		// checking if at least one thread is busy
		bool is_standby() const;

		// checking the execution of all tasks from the queue
		bool is_comleted() const
		{
			return completed_task_count == last_task_id;
		}

	};

	struct MassivePart 
	{
		long long int begin;
		long long int size;
	};

	void separate_massive(long long int full_size, long long int part_size, int thread_count, std::vector<MassivePart>& parts);
}

#endif