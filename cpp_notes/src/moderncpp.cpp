#include <chrono>
#include <future>
#include <iostream>
#include <random>
#include <vector>
using namespace std;

void test() {
  // lambda表达式
  int a = 1, b = 2;
  auto f = [=, &b]() { return a + b; };
  auto sum = f();

  //促使话列表
  float fnum{1.23};
  vector<int> v{1, 2, 3, 4};
}
// 尾随返回类型
auto add(int a, int b) -> decltype(a + b) { return a + b; }

// future (async, wait_for, get)
void test_future() {
  auto is_prime = [](int x) {
    for (int i = 2; i < (x); ++i)
      if (x % i == 0) return false;
    return true;
  };
  constexpr int num = 444444443;
  std::future<bool> fut = std::async(is_prime, num);

  cout << "checking, please wait";
  std::chrono::milliseconds span(100);
  while (fut.wait_for(span) == std::future_status::timeout) cout << '.' << std::flush;

  bool x = fut.get();  // retrieve return value
  cout << "\n" << num << " " << (x ? "is" : "is not") << " prime.\n";
}
// 伪随机数生成
void test_rand() {
  std::random_device rd;
  std::mt19937 rg(rd());
  std::uniform_int_distribution<> dist(1, 10);
  for (int n = 0; n < 10; n++) cout << dist(rg) << " ";
  cout << endl;

  std::normal_distribution norm_dist;
  for (int n = 0; n < 10; n++) cout << norm_dist(rg) << " ";
  cout << endl;
}

int main(int argc, char** argv) {
  cout << add(1, 2) << endl;
  test_rand();
  return 0;
}