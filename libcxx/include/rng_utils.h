#ifndef RNG_UTILS_H
#define RNG_UTILS_H
#include<random>

template<typename T> struct uniform_distribution {
  typedef void type; // error
};

template<> struct uniform_distribution<char> {
   typedef std::uniform_int_distribution<int> type;
};

template<> struct uniform_distribution<int> {
   typedef std::uniform_int_distribution<int> type;
};

template<> struct uniform_distribution<unsigned> {
   typedef std::uniform_int_distribution<unsigned> type;
};

template<> struct uniform_distribution<float> {
   typedef std::uniform_real_distribution<float> type;
};

template<> struct uniform_distribution<double> {
   typedef std::uniform_real_distribution<double> type;
};

template<class T> using gen = typename uniform_distribution<T>::type;

class random_device {
public:
  template<typename T>
  T get_rand(T min, T max) {
    std::mt19937 e(rd()); // seed the generator
    gen<T> d(min, max); // define the range
    return d(e);
  }

private:
  // TODO: Fix the seed to a constant.
  std::random_device rd; // obtain a random number from hardware
};


#endif // RNG_UTILS_H

