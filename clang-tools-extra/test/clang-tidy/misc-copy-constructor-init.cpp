// RUN: %check_clang_tidy %s misc-copy-constructor-init %t

class Copyable {
	public:
	Copyable() = default;
	Copyable(const Copyable&) = default;
};
class X : public Copyable {
	X(const X& other) : Copyable(other) {}
	//Good code: the copy ctor call the ctor of the base class.
};

class Copyable2 {
	public:
	Copyable2() = default;
	Copyable2(const Copyable2&) = default;
};
class X2 : public Copyable2 {
	X2(const X2& other) {};
    // CHECK-MESSAGES: :[[@LINE-1]]:22: warning: calling an inherited constructor other than the copy constructor [misc-copy-constructor-init]
	// CHECK-FIXES: : Copyable2(other)
};

class X3 : public Copyable, public Copyable2 {
	X3(const X3& other): Copyable(other) {};
	// CHECK-MESSAGES: :[[@LINE-1]]:23: warning: calling an inherited constructor other than the copy constructor [misc-copy-constructor-init]
	// CHECK-FIXES: Copyable2(other), 
};

class X4 : public Copyable {
	X4(const X4& other): Copyable() {};
	// CHECK-MESSAGES: :[[@LINE-1]]:32: warning: calling an inherited constructor other than the copy constructor [misc-copy-constructor-init]
	// CHECK-FIXES: other
};

class Copyable3 : public Copyable {
	public:
	Copyable3() = default;
	Copyable3(const Copyable3&) = default;
};
class X5 : public Copyable3 {
	X5(const X5& other) {};
    // CHECK-MESSAGES: :[[@LINE-1]]:22: warning: calling an inherited constructor other than the copy constructor [misc-copy-constructor-init]
	// CHECK-FIXES: : Copyable3(other)
};

class X6 : public Copyable2, public Copyable3 {
	X6(const X6& other) {};
	// CHECK-MESSAGES: :[[@LINE-1]]:22: warning: calling an inherited constructor other than the copy constructor [misc-copy-constructor-init]
	// CHECK-FIXES: : Copyable2(other), Copyable3(other)
};

class X7 : public Copyable, public Copyable2 {
	X7(const X7& other): Copyable() {};
	// CHECK-MESSAGES: :[[@LINE-1]]:32: warning: calling an inherited constructor other than the copy constructor [misc-copy-constructor-init]
	// CHECK-FIXES: other
	// CHECK-MESSAGES: :[[@LINE-3]]:22: warning: calling an inherited constructor other than the copy constructor [misc-copy-constructor-init]
	// CHECK-FIXES: Copyable2(other), 
};

template <class C>
class Copyable4 {
	public:
	Copyable4() = default;
	Copyable4(const Copyable4&) = default;
};

class X8 : public Copyable4<int> {
	X8(const X8& other): Copyable4<int>(other) {};
	//Good code: the copy ctor call the ctor of the base class.
};

class X9 : public Copyable4<int> {
	X9(const X9& other): Copyable4<int>() {};
	// CHECK-MESSAGES: :[[@LINE-1]]:37: warning: calling an inherited constructor other than the copy constructor [misc-copy-constructor-init]
	// CHECK-FIXES: other
};

class X10 : public Copyable4<int> {
	X10(const X10& other) {};
	// CHECK-MESSAGES: :[[@LINE-1]]:22: warning: calling an inherited constructor other than the copy constructor [misc-copy-constructor-init]
	// CHECK-FIXES: : Copyable4<int>(other)
};

template <class T, class S>
class Copyable5 {
	public:
	Copyable5() = default;
	Copyable5(const Copyable5&) = default;
};

class X11 : public Copyable5<int, float> {
	X11(const X11& other) {};
	// CHECK-MESSAGES: :[[@LINE-1]]:22: warning: calling an inherited constructor other than the copy constructor [misc-copy-constructor-init]
	// CHECK-FIXES: : Copyable5<int, float>(other)
};
