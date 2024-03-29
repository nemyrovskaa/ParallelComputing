#include "ThreadPool.h"

ThreadPool::ThreadPool()
{

}

ThreadPool::~ThreadPool()
{
	break_safe();
}

void ThreadPool::init()
{
	unique_lock<mutex> u_lock(m_rw_mutex);
	if (m_initialised)
		return;

	m_break_execution = m_is_paused = false;
	m_initialised = true;
	min_time = max_time = 0;

	m_queue.set_max_size(MAX_Q_SIZE);
	m_threads.reserve(MAX_THREAD_NUM);

	for (int i = 0; i < MAX_THREAD_NUM; i++)
		m_threads.push_back(thread(&ThreadPool::routine, this));
}

void ThreadPool::add_task(Task* task)
{
	if(m_queue.push(task))
	{
		//unique_lock<mutex> u_lock(m_clock_mutex);
		unique_lock<mutex> u_lock(m_rw_mutex);
		if (m_queue.get_size() == MAX_Q_SIZE)
			m_start_time = high_resolution_clock::now();
	}
	else
	{
		unique_lock<mutex> u_lock(m_rw_mutex);
			task->set_status(Status::IGNORED);
	}
	m_wait_for_task.notify_all();
}

void ThreadPool::routine()
{
	while (true)
	{
		Task* task = nullptr;
		{
			unique_lock<mutex> u_lock(m_rw_mutex);
			m_wait_for_task.wait(u_lock, [this] {return (!m_queue.empty() || m_break_execution) && !m_is_paused; });

			if (m_break_execution)
				return;

			try
			{
				{
					//unique_lock<mutex> u_lock(m_clock_mutex);
					if (m_queue.get_size() == MAX_Q_SIZE)
						m_end_time = high_resolution_clock::now();

					unsigned long long time = duration_cast<milliseconds>(m_end_time - m_start_time).count();
					if (max_time == 0)
						min_time = max_time = time;
					else if (time < min_time)
						min_time = time;
					else if (time > max_time)
						max_time = time;
				}
				task = m_queue.pop();
			}
			catch (out_of_range& e)
			{
				cout << e.what() << endl;
			}
		}
		task->do_work();
	}
}

void ThreadPool::break_safe()
{
	{
		unique_lock<mutex> u_lock(m_rw_mutex);
		m_break_execution = true;
	}

	m_wait_for_task.notify_all();
	for (thread &i : m_threads)
		if(i.joinable())
			i.join();

	m_threads.clear();
	m_queue.clear();
	m_initialised = false;
}

void ThreadPool::break_momentary()
{
	{
		unique_lock<mutex> u_lock(m_rw_mutex);
		m_break_execution = true;
	}

	m_wait_for_task.notify_all();
	for (thread& i : m_threads)
		if(i.joinable())
			i.detach();

	m_threads.clear();
	m_queue.clear();
	m_initialised = false;
}

void ThreadPool::pause()
{
	unique_lock<mutex> u_lock(m_pause_mutex);
	m_is_paused = true;
}

void ThreadPool::resume()
{
	{
		unique_lock<mutex> u_lock(m_pause_mutex);
		m_is_paused = false;
	}
	m_wait_for_task.notify_all();
}