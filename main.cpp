#include <iostream>
#include <vector>
#include <functional>

#include <execution>
#include <numeric>

#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>

using namespace std;

template <class F>
void timeit(F &&f)
{
    auto time1 = chrono::system_clock::now();
    f();
    auto time2 = chrono::system_clock::now();
    auto tdiff = time2 - time1;
    cout << tdiff.count() << "ns" << endl;
}

vector<int> getNumbers(int n)
{
    vector<int> numbers;
    for (int i = 0; i < n; ++i)
    {
        numbers.push_back(i);
    }
    return numbers;
}

auto is_even = [](int x) { return x % 2 == 0; };

template <typename T>
void policiesTest(T& data) {
    cout << "no policy: ";
    timeit([&data]() {
        count_if(data.begin(), data.end(), is_even);
    });

    cout << "sequential: ";
    timeit([&data]() {
        count_if(execution::seq, data.begin(), data.end(), is_even);
    });

    cout << "parallel: ";
    timeit([&data]() {
        count_if(execution::par, data.begin(), data.end(), is_even);
    });

    cout << "unsequential: ";
    timeit([&data]() {
        count_if(execution::unseq, data.begin(), data.end(), is_even);
    });

    cout << "paralel unseq: ";
    timeit([&data]() {
        count_if(execution::par_unseq, data.begin(), data.end(), is_even);
    });
}

template <typename Iterator, typename T>
struct count_if_wrap
{
    void operator()(Iterator first, Iterator last, T &result)
    {
        result = count_if(first, last, is_even);
    }
};

template <typename Iterator>
void own_parallel(Iterator first, Iterator last, unsigned int K=1)
{
    unsigned long const length = distance(first, last);

    unsigned long const block_size = length / K;
    vector<thread> threads(K-1 );
    vector<int> results(K, 0);
    Iterator block_start = first;
    for (unsigned long i = 0; i < (K-1); ++i)
    {
        Iterator block_end = block_start;
        advance(block_end, block_size);
        threads[i] = thread(count_if_wrap<Iterator, int>(), block_start,
                            block_end, ref(results[i]));
        block_start = block_end;
    }
    count_if_wrap<Iterator, int>()(block_start, last, results[K - 1]);
    for (auto &entry : threads)
        entry.join();
}

int main() {
    auto data_small = getNumbers(100000);
    auto data_medium = getNumbers(1000000);
    auto data_large = getNumbers(10000000);
    unsigned int const hardware_threads = thread::hardware_concurrency();
    unsigned int const max_threads = hardware_threads ? hardware_threads : 2;

    cout<<"Small data test:"<<endl;
    policiesTest(data_small);

    cout<<"\nMedium data test:"<<endl;
    policiesTest(data_medium);

    cout<<"\nLarge data test:"<<endl;
    policiesTest(data_large);

    cout<<"\nOwn parallel algorithm:"<<endl;
    for(unsigned int K=1; K<=max_threads; ++K){
        cout<<"K=" << left << setw(5) <<K;
        timeit([&data_small, &K]()
               { own_parallel(data_small.cbegin(), data_small.cend(), K); });
    }

    return 0;
}
