#pragma once

#include <iostream>
#include <thread>
#include <vector>

#define CALLBACK_ABORT 0
#define CALLBACK_OK    1
#define CALLBACK_PAUSE 2

class ICallback
{
public:
	virtual ~ICallback() {} ;
	virtual void start() = 0;
	virtual void finish() = 0;
	/** Return values:
	* 0 - user aborted the process. need to stop/abort current operation
	* 1 - everything is ok, continue operation
	* 2 - pause operation (call method progess() with the last parameter every 1 second until any other value than 2 returned)
	*/
	virtual int progress(uint64_t prgrs) = 0;

};

/**
* This class used for showing ONLY progress on the console
* It shows one line without '\n' at the end and updates info on this line each time when progress() method is called
*/
class ConsoleProgressCallback: public ICallback
{
public:

	void start() override {}
	void finish() override { std::cout << "\n"; }
	int progress(uint64_t prgrs) override
	{
		std::cout << "\rProgress " << prgrs << "%...";
		return CALLBACK_OK;
	}

};

class CallbackManager
{
private:
	std::vector<ICallback*> callbacks;
public:
	void AddCallback(ICallback* cb)
	{
		auto iter = std::find(callbacks.begin(), callbacks.end(), cb);
		if (iter == callbacks.end()) // not found
		{
			callbacks.push_back(cb);
		}
		// do nothing if callback is already in the list.
	}

	void RemoveCallback(ICallback* cb)
	{
		auto iter = std::find(callbacks.begin(), callbacks.end(), cb);
		if (iter != callbacks.end())
		{
			callbacks.erase(iter);
		}
	}

	void Start()
	{
		for (auto item : callbacks) item->start();
	}

	void Finish()
	{
		for (auto item : callbacks) item->finish();
	}

	int Progress(uint64_t prg)
	{
		for (auto item : callbacks)
		{
			int res = item->progress(prg);

			if (res == CALLBACK_PAUSE)
			{
				do
				{
					std::this_thread::sleep_for(std::chrono::seconds(1));
				} while ((res = item->progress(prg)) == CALLBACK_PAUSE);
			}

			if (res == CALLBACK_ABORT) return CALLBACK_ABORT;
		}

		return CALLBACK_OK;
	}
};