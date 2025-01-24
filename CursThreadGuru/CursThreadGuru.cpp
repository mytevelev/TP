// CursThreadGuru.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//
// количество касс ограниченное количеством потоков разгребает очередь из покупателей за разное для каждой кассы время.

#include <iostream>
#include <queue>
#include <chrono>
#include <thread>
#include <random>
#include <condition_variable>
#include <functional>

using namespace std;
using namespace chrono;

using SimpleFun = function<void( int)>; // все покупатели одинаковые


class safe_queue
{
public:
	void push(SimpleFun f)
	{
		lock_guard<mutex> lock(m);
		q.push(f);
		
		cv.notify_one();
	}

	void pop(SimpleFun& f)
	{
		unique_lock<mutex> lock(m);
		cv.wait(lock, [this] { return !q.empty(); });

			f = q.front(); // get the function from the front of the queue
			q.pop(); // remove the function from the queue
			
		
	}

	size_t  size() const
	{
		return q.size();
	};

private:
	queue<SimpleFun> q;
	mutex m;
	condition_variable cv;
	
};


// generate clients

int GenClients( int MaxClients )
{ 

	return rand( )%MaxClients+1;

}

mutex m_c;
void StandardClient( int s ) // стандартный клиент
 {
	unique_lock<mutex> lock(m_c);
	cout << " "<<s;
 };

class thread_pool
{
	private:
		atomic <int> p_clients = 0;
		bool done_flag = false;
		vector<thread> cashiers;
		// consumer thread
		safe_queue myq;
		void Release(int cashID, int delay)	// поток для касс
		{
			while (myq.size() > 0 || !done_flag)
			{
				SimpleFun f;
				myq.pop(f);
				f(cashID);
				p_clients++;
				this_thread::sleep_for(chrono::milliseconds(delay));
			};
		};



	public:

		thread_pool()
		{
			unsigned int num_cores = std::thread::hardware_concurrency();
			cout << "Number of cores: " << num_cores << endl;
			for (unsigned int i = 0; i < num_cores; i++)
			{
				int del = 100 * (rand() %( i + 1));
				thread t(&thread_pool::Release, this, i + 1, del); //??? ???? 100ms * (i+1)
				cashiers.push_back(std::move(t));
				
			}
			cout << "Cashiers created!\n";
		};
		~thread_pool()
		{
			for (auto& t : cashiers)
				t.join();
		};

		void submit(SimpleFun f)
		{
			myq.push(f);

		};

		size_t size() const
		{
			return myq.size(); 
		}

		size_t processed_clients() const
		{
			return p_clients; 
		}

		void setDoneFlag()
		{
			done_flag = true;  
		};


};




 int main()
 {
	 
	 int c_total = 0;

	 thread_pool mypool;

	 srand( static_cast<unsigned int> (time(NULL)) );


	 for (int i = 0; i < 10; i++)
	 {
		 int ct = GenClients(40);  c_total += ct;
		 for (int j = 0; j < ct; j++)
		 {

			 mypool.submit(StandardClient);
		 };
		 {
			 unique_lock<mutex> lock(m_c);
			 cout << "\nGenerated " << ct << " clients. Queue size: " << mypool.size() << " Processed: " << mypool.processed_clients() << endl;
		 }
			 // sleep for one second
			 this_thread::sleep_for(chrono::seconds(1));
		 

	 }

	 mypool.setDoneFlag();  // signal consumer thread to finish processing . cashier ON!
	 cout << "\nTotal clients generated: " << c_total << endl;

	 while (mypool.size())
	 {
		 this_thread::sleep_for(chrono::milliseconds(100));
	 };

	 cout << "\nAll clients released: " << mypool.processed_clients() << " Queue size: " << mypool.size() << endl;


	 cout << "Program finished." << endl;
 };
