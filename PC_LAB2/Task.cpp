#include "Task.h"

unsigned int Task::s_ID = 0;

Task::Task()
{
	m_taskID = ++s_ID;
	m_status = Status::IS_STORING;
	m_duration_ms = MIN_TASK_TIME + rand() % (MAX_TASK_TIME - MIN_TASK_TIME + 1);
}

Task::~Task()
{

}

void Task::do_work()
{
	set_status(Status::IS_PROCESSING);
	m_start_time = chrono::high_resolution_clock::now();

	Sleep(m_duration_ms);

	m_end_time = chrono::high_resolution_clock::now();
	m_result_time = chrono::duration_cast<chrono::milliseconds>(m_end_time - m_start_time);
	set_status(Status::EXECUTED);
}

unsigned int Task::get_ID()
{
	return m_taskID;
}

string Task::get_string()
{
	string ret_str;
	ret_str = string("Task [") + to_string(m_taskID) + string("] - Duration(") + 
		to_string(m_result_time.count()) + string(" ms) ") + get_status_str() + "\n";
	return ret_str;
}

void Task::set_status(Status status)
{
	unique_lock<shared_mutex> u_lock(m_rw_mutex);
	m_status = status;
}

Status Task::get_status()
{
	shared_lock<shared_mutex> s_lock(m_rw_mutex);
	return m_status;
}