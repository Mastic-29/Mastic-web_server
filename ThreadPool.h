#pragma once

// -----------------------------------------------------------------------*/
//
//文件名：ThreadPool.h
//创建人：MasticZhang
//创建时间：2017/7/21 09：54
//功能描述：线程池
//
// -----------------------------------------------------------------------*/

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include  <queue>
#include <list>
#include <windows.h>
#include <process.h>

namespace MasticThreadPool {
	//任务类
	class Itask
	{
	public:
		Itask() {}
		virtual ~Itask() {}

	public:
		virtual void RunItask() = 0;
	};

	//线程池类
	class ThreadPool
	{
	public:
		ThreadPool()
			: lRunThreadNum(0)
			, lMaxThreadNum(0)
			, lCreateThreadNum(0)
			, hSemphore(nullptr)
			, bFlagQuit(FALSE) {
			InitializeCriticalSection(&cs);
		}

		~ThreadPool() {
			DestroyThreadPool();
		}

	public:

		// -----------------------------------------------------------------------*/
		//函数方法：CreateThreadPool
		//作用：创建线程池
		// -----------------------------------------------------------------------*/
		BOOL CreateThreadPool(long lMaxThreadNum) {
			if (lMaxThreadNum <= 0) {
				return FALSE;
			}

			hSemphore = CreateSemaphore(nullptr, 0, lMaxThreadNum, nullptr);
			bFlagQuit = TRUE;

			for (long loop_i = 0; loop_i < lMaxThreadNum; loop_i++) {
				HANDLE hThread = CreateThread(nullptr, 0, &ThreadProc, this, 0, nullptr);
				if (hThread) {
					list_Thread.push_back(hThread);
				}
			}
			this->lMaxThreadNum = lMaxThreadNum;

			return TRUE;
		}



		// -----------------------------------------------------------------------*/
		//函数方法：DestroyThreadPool
		//作用：销毁线程池
		// -----------------------------------------------------------------------*/
		void DestroyThreadPool() {
			bFlagQuit = FALSE;
			ReleaseSemaphore(hSemphore, lCreateThreadNum, nullptr);
			Sleep(100);

			std::list<HANDLE>::iterator iteThread;
			for (iteThread = list_Thread.begin(); iteThread != list_Thread.end(); iteThread++) {
				if (WAIT_TIMEOUT == WaitForSingleObject(*iteThread, 1000)) {
					TerminateThread(*iteThread, -1);
				}
				if (*iteThread) {
					CloseHandle(*iteThread);
					*iteThread = nullptr;
				}
			}//for end

			if (hSemphore) {
				CloseHandle(hSemphore);
				hSemphore = nullptr;
			}

			Itask *pItask{ nullptr };
			while (!queue_Itask.empty()) {
				pItask = queue_Itask.front();
				queue_Itask.pop();
				delete pItask;
			}

			DeleteCriticalSection(&cs);
		}
		
		// -----------------------------------------------------------------------*/
		//函数方法：ThreadProc
		//作用：线程处理函数
		// -----------------------------------------------------------------------*/
		static DWORD WINAPI ThreadProc(LPVOID lpThreadParameter) {
			ThreadPool *pthis = (ThreadPool *)lpThreadParameter;
		
			while (pthis->bFlagQuit) {
				WaitForSingleObject(pthis->hSemphore, INFINITE);

				InterlockedIncrement(&pthis->lRunThreadNum);

				while (pthis->bFlagQuit) {//循环处理问题 不退出
					Itask *task{ nullptr };

					EnterCriticalSection(&(pthis->cs));
					
					if (!(pthis->queue_Itask).empty()) {
						task = (pthis->queue_Itask).front();
						(pthis->queue_Itask).pop();
					}
					else {
						LeaveCriticalSection(&(pthis->cs));
						break;
					}
					
					LeaveCriticalSection(&(pthis->cs));
					
					task->RunItask();
					delete task;
				}

				InterlockedDecrement(&pthis->lRunThreadNum);
			}

			return 0;
		}


		// -----------------------------------------------------------------------*/
		//函数方法：PushItask
		//作用：向线程池投递任务
		// -----------------------------------------------------------------------*/
		BOOL PushItask(Itask *task) {
			if (!task) {
				return FALSE;
			}

			//压任务入队
			EnterCriticalSection(&cs);
			queue_Itask.push(task);
			LeaveCriticalSection(&cs);

			//释放信号量
			if (lRunThreadNum < lCreateThreadNum) {
				ReleaseSemaphore(hSemphore, 1, nullptr);
			}
			else if (lCreateThreadNum < lMaxThreadNum) {
				HANDLE hThread = CreateThread(nullptr, 0, &ThreadProc, this, 0, nullptr);
				if (hThread) {
					list_Thread.push_back(hThread);
				}
				lCreateThreadNum++;

				ReleaseSemaphore(hSemphore, 1, nullptr);
			}
			else {
				//等待 
			}
		
			return TRUE;
		}

	private:
		std::queue<Itask*> queue_Itask;//任务队列
		std::list<HANDLE> list_Thread;//线程队列

		long lRunThreadNum;
		long lMaxThreadNum;
		long lCreateThreadNum;

		CRITICAL_SECTION cs;//关键段
		HANDLE hSemphore;//信号量

		BOOL bFlagQuit;
	};

}

#endif // THREADPOOL_H_