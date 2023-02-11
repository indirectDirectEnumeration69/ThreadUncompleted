#pragma once
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <windows.h>
#include <string>
#include <algorithm>
#include <set>
#include <cstdint>
#include <iostream>
#include <thread>
#include <tchar.h>
#include <intrin.h>
#include <chrono>
#include <memory>
#include <DbgHelp.h> 
#pragma comment(lib, "DbgHelp.lib")
class WOzzing {
public:
	constexpr static int MAX_THREADS = 8;
	WOzzing() {
		for (int i = 0; i < MAX_THREADS / 2; i++) {
			threads.push_back(std::make_unique<std::thread>(&WOzzing::timing, this));
		}
		for (int i = threads.size(); i < MAX_THREADS; i++) {
			threads.push_back(std::make_unique<std::thread>(&WOzzing::systemrun, this));
		}
	}

	std::atomic<int> threadRunCount{ 0 };
	void timing() {
		std::unordered_set<std::string, std::hash<std::string>, std::equal_to<std::string>> time;
		std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

		while (threadRunCount < MAX_THREADS) {
			std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
			std::chrono::duration<double> elapsed_seconds = end - start;
			std::vector<std::string> Time;

			if (elapsed_seconds.count() > 10) {
				time.clear();
				std::string size_string = std::to_string(time.size());
				Time.push_back(size_string);
				start = std::chrono::system_clock::now();
			}
			time.insert("Time");
			if (threadRunCount >= MAX_THREADS) {
				break;
			}
		}
		threadRunCount++;
	}
	void systemrun() {
		constexpr int MAX_THREAD2 = 10;
		constexpr byte MAX_PROCESSORS = 4;
		//* to do for finding the processors running on the system.
		class Change {
		public:
			Change(int Tasks) : tasks(Tasks) {}

			auto operator()() const { //functor 
				auto EventLogTasks = [=]() -> std::vector<int> {
					std::vector<int> EventLog;
					for (int i = 0; i < tasks; i++) {
						EventLog.push_back(i);
					}
					return EventLog;
				};

				std::vector<std::unique_ptr<std::thread>> threads;
				for (int i = 0; i < MAX_THREAD2; i++) {
					int Jobs = EventLogTasks()[i % EventLogTasks().size()];
					threads.emplace_back(std::make_unique<std::thread>([&]() { //lam expression
						for (int j = 0; j < Jobs; j++) {
							std::this_thread::sleep_for(std::chrono::milliseconds(100));
							std::this_thread::yield(); // yeild time to other threads
						}
						}));
				}

				for (auto& o : threads) {
					o->join();
				}
			}

		private:
			int tasks;
		};

		struct SystemInfo {
			DWORD numberOfProcessors;
			DWORD processorType;
			DWORD activeProcessorMask;
			DWORD pageSize;
			LPVOID lpMinimumApplicationAddress;
			LPVOID lpMaximumApplicationAddress;
			std::thread::id threadId = std::this_thread::get_id();
		};

		class ErrorLog {
		public:
			enum class StatusCheck {
				FULL,
				EMPTY,
				HALF_FULL,
				CAPACITY,
				NOT_FOUND,
				FOUND,
				CAPACITY_REACHED
			}; 
			enum class EnvironmentCheck {
				ENVIRONMENT_VIRTUAL,
				ENVIRONMENT_HOST,
				ENVIRONMENT_DEBUG,
				ENVIRONMENT_RELEASE,
				ENVIRONMENT_TEST,
				ENVIRONMENT_DEVELOPMENT,
				ENVIRONMENT_PRODUCTION,
				ENVIRONMENT_STAGING,
				ENVIRONMENT_DEPLOYMENT,
				ENVIRONMENT_DEPLOYMENT_TEST,
				ENVIRONMENT_DEPLOYMENT_PRODUCTION,
				ENVIRONMENT_DEPLOYMENT_STAGING,
				ENVIRONMENT_FINISHED,
				FOUND_ENVIRONMENT,
				NOT_FOUND_ENVIRONMENT
			};
			int TasksRunning = 0;
			int Task = 0;
			std::unordered_map<std::string, std::string> statusLog;
			std::unordered_map<std::string, std::string> environmentLog;	
			ErrorLog() {
				constexpr int HALF = MAX_THREAD2 / 2;
				for (auto i = 0; i < HALF; i++) {
					Threads.push_back(std::make_unique<std::thread>(&ErrorLog::CollectErrors, this));
					std::cout << "Collecting Errors" << std::endl;
				}
				while (Threads.size() <= MAX_THREAD2) {
					Threads.push_back(std::make_unique<std::thread>(&ErrorLog::sysadvcinfo, this));
					int ThreadNum = Threads.size();
					TasksRunning++;
					errors.rehash(1000);
					if (errors.find("Error") != errors.end() || errors.find("Errors") != errors.end()) {
						std::cout << "Error Found" << std::endl;
						error.insert(errors.find("Error")->second);
					}
					else {
						Threads.begin()->release();
						Threads.data()->~unique_ptr();
						Threads.pop_back();
						Threads.shrink_to_fit();
						Threads.clear();
						Threads.erase(Threads.begin(), Threads.end());
					}
				}
				Change EventErrorInternal(TasksRunning);
				EventErrorInternal();
				auto iterLogStats = std::inserter(statusLog, statusLog.begin());
				*iterLogStats = std::make_pair("System Status:", std::to_string(static_cast<int>(status(statusLog))));
			}
		public:
			void CollectErrors() {
				HANDLE hEventLog = OpenEventLog(NULL, L"System");
				if (hEventLog != NULL)
				{
					DWORD dwRead = 0, dwNeeded = 0;
					std::vector<BYTE> buffer(4096);
					if (ReadEventLog(hEventLog, EVENTLOG_FORWARDS_READ | EVENTLOG_SEQUENTIAL_READ, 0, buffer.data(), buffer.size(), &dwRead, &dwNeeded))
					{
						EVENTLOGRECORD* pEvent = (EVENTLOGRECORD*)buffer.data();
						while (dwRead > 0)
						{
							std::string sourceName((LPCSTR)(pEvent + 1));
							std::string message((LPCSTR)((BYTE*)pEvent + pEvent->StringOffset));
							errors.insert({ sourceName, message });
							dwRead -= pEvent->Length;
							pEvent = (EVENTLOGRECORD*)(BYTE*)pEvent + pEvent->Length;
						}
					}
					CloseEventLog(hEventLog);
				}
			}
			void sysadvcinfo() {

				std::unordered_multiset<std::pair<int, SYSTEM_INFO>> sysSet;
				auto hProcess = GetCurrentProcess();
				auto hThread = GetCurrentThread();
				auto dwProcessId = GetCurrentProcessId();
				for (int i = 0; i < MAX_PROCESSORS; i++) {
					SYSTEM_INFO sysInfomation = { 0 };
					GetSystemInfo(&sysInfomation);
					sysSet.insert(std::make_pair(dwProcessId, sysInfomation));
					i = sysInfomation.dwNumberOfProcessors;
					for (sysSet.begin(); sysSet.end() != sysSet.end(); i++)
					{

					}
				}
			}
		private:
			std::vector<std::unique_ptr<std::thread>> Threads; //the threads that are gonna be used for the wowbot system  ..
			std::unordered_map<std::thread, std::thread::id> ThreadIDs; //ids should then now get ids()
			std::unordered_map<std::string, std::string> errors; //the map of errors from the unordered set/
			std::unordered_set<std::string> error; ////added to first.
			StatusCheck statuss(std::unordered_map<std::string, std::string> ThreadErrors){
				
			}
			EnvironmentCheck environment(std::unordered_map<std::string, std::string> ThreadErrorsEnv) {
				return EnvironmentCheck::FOUND_ENVIRONMENT;
			}
		};
		class StatusChecking:ErrorLog {

			ErrorLog::StatusCheck statuss(std::unordered_map<std::string, std::string> ThreadErrors) {
				if (ThreadErrors.size() >= 1000) {
					return ErrorLog::StatusCheck::FULL;
				}
				else if (ThreadErrors.size() <= 0) {
					return ErrorLog::StatusCheck::EMPTY;
				}
				else if (ThreadErrors.size() >= 500) {
					return ErrorLog::StatusCheck::HALF_FULL;
				}
			}
			int checkStatus(StatusCheck status) {
				
				switch (status) {
				case StatusCheck::FULL:
					std::cout << "The system is full" << std::endl;
					statusLog.insert({ "system is full:", std::to_string(static_cast<int>(status)) });
					break;

				case StatusCheck::EMPTY:
					std::cout << "The system is empty" << std::endl;
					statusLog.insert({ "system empty:", std::to_string(static_cast<int>(status)) });
					break;

				case StatusCheck::HALF_FULL:
					std::cout << "The system is half full" << std::endl;
					statusLog.insert({ "system Half full:", std::to_string(static_cast<int>(status)) });
					break;

				case StatusCheck::CAPACITY:
					std::cout << "The system's current capacity:" << std::endl;
					statusLog.insert({ "system capacity:", std::to_string(static_cast<int>(status)) });
					break;

				case StatusCheck::NOT_FOUND:
					std::cout << "The system is not found" << std::endl;
					statusLog.insert({ "NOT FOUND:", std::to_string(static_cast<int>(status)) });
					break;

				case StatusCheck::FOUND:
					std::cout << "The system is found" << std::endl;
					statusLog.insert({ "System successfully found:", std::to_string(static_cast<int>(status)) });
					break;

				case StatusCheck::CAPACITY_REACHED:
					std::cout << "The system is at capacity reached" << std::endl;
					statusLog.insert({ "System has now reached capacity", std::to_string(static_cast<int>(status)) });
					break;

				default:
					break;
				}
			}
	 

		};
		/*
		ErrorLogCheck log;
		log.status();
	*/}
private:
	std::vector<std::unique_ptr<std::thread>> threads;
};

/*
ERRORS:

Severity	Code	Description	Project	File	Line	Suppression State
Error	C2280	'std::_Uhash_compare<_Kty,_Hasher,_Keyeq>::_Uhash_compare(const std::_Uhash_compare<_Kty,_Hasher,_Keyeq> &)': attempting to reference a deleted function	W0WB0T	C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.34.31933\include\unordered_set	49



//dont delete yet! ---> UNIQUE_PTR
*/