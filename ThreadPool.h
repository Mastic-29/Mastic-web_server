#pragma once

// -----------------------------------------------------------------------*/
//
//�ļ�����ThreadPool.h
//�����ˣ�MasticZhang
//����ʱ�䣺2017/7/21 09��54
//�����������̳߳�
//
// -----------------------------------------------------------------------*/

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include  <queue>
#include <list>
#include <windows.h>
#include <process.h>

namespace MasticThreadPool {
	//������
	class Itask
	{
	public:
		Itask() {}
		virtual ~Itask() {}

	public:
		virtual void RunItask() = 0;
	};

	//�̳߳���
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
		//����������CreateThreadPool
		//���ã������̳߳�
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
		//����������DestroyThreadPool
		//���ã������̳߳�
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
		//����������ThreadProc
		//���ã��̴߳�����
		// -----------------------------------------------------------------------*/
		static DWORD WINAPI ThreadProc(LPVOID lpThreadParameter) {
			ThreadPool *pthis = (ThreadPool *)lpThreadParameter;
		
			while (pthis->bFlagQuit) {
				WaitForSingleObject(pthis->hSemphore, INFINITE);

				InterlockedIncrement(&pthis->lRunThreadNum);

				while (pthis->bFlagQuit) {//ѭ���������� ���˳�
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
		//����������PushItask
		//���ã����̳߳�Ͷ������
		// -----------------------------------------------------------------------*/
		BOOL PushItask(Itask *task) {
			if (!task) {
				return FALSE;
			}

			//ѹ�������
			EnterCriticalSection(&cs);
			queue_Itask.push(task);
			LeaveCriticalSection(&cs);

			//�ͷ��ź���
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
				//�ȴ� 
			}
		
			return TRUE;
		}

	private:
		std::queue<Itask*> queue_Itask;//�������
		std::list<HANDLE> list_Thread;//�̶߳���

		long lRunThreadNum;
		long lMaxThreadNum;
		long lCreateThreadNum;

		CRITICAL_SECTION cs;//�ؼ���
		HANDLE hSemphore;//�ź���

		BOOL bFlagQuit;
	};

}

#endif // THREADPOOL_H_