#include "c++.hh"
#undef NULL
#undef __DATE__
#undef __TIME__
#define t__LINE__ t##__LINE__

//虚指令 编译预处理会被注释掉

//必须要有这个命令，编译指示不包括本文件
#pragma noinclude

//假设，虚指令 用途:兼容C++代码编辑器识别
//示例 assume int a;
#define assume

#pragma region 脚本系统定义

using int8 = char;
using int16 = short;
using int32 = int;
using int64 = long long;
using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;
using uint = unsigned int;

typedef int8 int8_t;
typedef int16 int16_t;
typedef int32 int32_t;
typedef int64 int64_t;
typedef uint8 uint8_t;
typedef uint16 uint16_t;
typedef uint32 uint32_t;
typedef uint64 uint64_t;
typedef uint uint_t;

template<typename T>
assume struct iterator
{
	iterator& operator++();
	T& operator*();
	const T& operator*() const;
	bool operator!=(iterator&);
};

template<typename T>
struct array
{
	using iter = iterator<T>;
	array();
	//array(const array&) = delete;
	array(uint length);
	array(uint length, const T& value);

	T& operator[](const uint index);
	const T& operator[](const uint index)const;

	array<T>& operator=(const array<T>&);

	void insertAt(uint index, const T& value);
	void insertAt(uint index, const array<T>& arr);
	void insertLast(const T& value);
	void removeAt(uint index);
	void removeLast();
	void removeRange(uint start, uint count);
	uint length() const;
	void reserve(uint length);
	void resize(uint length);
	void sortAsc();
	void sortAsc(uint startAt, uint count);
	void sortDesc();
	void sortDesc(uint startAt, uint count);
	void reverse();
	int find(const T& value) const;
	int find(uint startAt, const T& value) const;
	int findByRef(const T& value) const;
	int findByRef(uint startAt, const T& value) const;
	bool operator==(const array<T>&) const;
	bool operator!=(const array<T>&) const;
	bool isEmpty() const;
	void sort(std::Function<bool(const T& a, const T& b)>, uint startAt = 0, uint count = uint(-1));

	assume iter begin()const;
	assume iter end()const;
	assume array(std::initializer_list<T>);
};

struct string
{
	string();
	string(const string&);
	string(const char*);

	uint length() const;

	void resize(uint);

	bool isEmpty() const;

	string& operator=(const string&);
	string& operator+=(const string&);
	string operator+(const string&) const;

	bool operator==(const string&) const;
	bool operator!=(const string&) const;
	bool operator>(const string&) const;
	bool operator<(const string&) const;
	bool operator>=(const string&) const;
	bool operator<=(const string&) const;

	uint8& operator[](const uint);
	const uint8& operator[](const uint) const;

	string& operator=(const char*);
	string& operator+=(const char*);
	string operator+(const char*) const;

	string& operator=(const double);
	string& operator+=(const double);
	string operator+(const double) const;

	string& operator=(const float);
	string& operator+=(const float);
	string operator+(const float) const;

	string& operator=(const int64);
	string& operator+=(const int64);
	string operator+(const int64) const;

	string& operator=(const uint64);
	string& operator+=(const uint64);
	string operator+(const uint64) const;

	string& operator=(const int);
	string& operator+=(const int);
	string operator+(const int) const;

	string& operator=(const uint);
	string& operator+=(const uint);
	string operator+(const uint) const;

	string& operator=(const int16);
	string& operator+=(const int16);
	string operator+(const int16) const;

	string& operator=(const uint16);
	string& operator+=(const uint16);
	string operator+(const uint16) const;

	string& operator=(const int8);
	string& operator+=(const int8);
	string operator+(const int8) const;

	string& operator=(const uint8);
	string& operator+=(const uint8);
	string operator+(const uint8) const;

	string& operator=(const bool);
	string& operator+=(const bool);
	string operator+(const bool) const;

	string substr(uint start = 0, int count = -1) const;
	int findFirst(const string&, uint start = 0) const;
	int findFirstOf(const string&, uint start = 0) const;
	int findFirstNotOf(const string&, uint start = 0) const;
	int findLast(const string&, int start = -1) const;
	int findLastOf(const string&, int start = -1) const;
	int findLastNotOf(const string&, int start = -1) const;
	int regexFind(const string& regex, uint start = 0, uint& lengthOfMatch) const;
	void insert(uint pos, const string& other);
	void erase(uint pos, int count = -1);

	array<string>& split(const string& 分割符) const;

	//扩展类对象方法
	string takeSubStr(const string& 分割符);
	void replace(const string& 欲替换的字符串, const string& 替换为的字符串);
	int count(const string& 子串);

	assume null_t operator=(null_t) = delete;
	assume string(const std::Function<void(void)>&);
};

//string::split的反函数
string join(const array<string>& 字符串数组, const string& 分割符);

//示例 uint scanned = scan("123 3.14 hello", i, f, s); i = 123, f = 3.14, s = hello;
template<typename... type>
uint scan(const string& str, type& ...);

//示例 string result = format("{} {} {}", 123, true, 'hello'); result = "123 true hello"
template<typename... type>
string format(const string& fmt, const type& ...);

string formatInt(int64 val, const string& options, uint width = 0);
string formatUInt(uint64 val, const string& options, uint width = 0);
string formatFloat(double val, const string& options, uint width = 0, uint precision = 0);
int64 parseInt(const string&, uint base = 10, assume const uint& byteCount = 10);
uint64 parseUInt(const string&, uint base = 10, assume const uint& byteCount = 10);
double parseFloat(const string&, assume const uint& byteCount = 10);

double cos(double);
double sin(double);
double tan(double);
double acos(double);
double asin(double);
double atan(double);
double atan2(double, double);
double cosh(double);
double sinh(double);
double tanh(double);
double log(double);
double log10(double);
double pow(double, double);
double sqrt(double);
double ceil(double);
double abs(double);
double floor(double);
double fraction(double);

struct complex
{
	complex();
	complex(const complex& other);
	complex(float r, float i = 0);

	// Assignment operator
	complex& operator=(const complex& other);

	// Compound assigment operators
	complex& operator+=(const complex& other);
	complex& operator-=(const complex& other);
	complex& operator*=(const complex& other);
	complex& operator/=(const complex& other);

	float length() const;
	float squaredLength() const;

	// Swizzle operators
	complex get_ri() const;
	void    set_ri(const complex&);
	complex get_ir() const;
	void    set_ir(const complex&);

	// Comparison
	bool operator==(const complex& other) const;
	bool operator!=(const complex& other) const;

	// Math operators
	complex operator+(const complex& other) const;
	complex operator-(const complex& other) const;
	complex operator*(const complex& other) const;
	complex operator/(const complex& other) const;

	assume null_t operator=(null_t) = delete;
};

#define catch catch(...)
#define throw _throw
void throw(const string&);
string getExceptionInfo();

struct dictionary
{
	struct value
	{
		template<typename type>
		value(const type&);

		template<typename type>
		explicit operator type() const;

		template<typename type>
		value& operator=(const type&);
	};

	using iter = iterator<value>;

	dictionary();
	//dictionary(const dictionary&) = delete;
	dictionary& operator=(const dictionary&);

	template<typename type>
	void set(const string& key, const type& value);
	template<typename type>
	bool get(const string& key, type& value) const;
	bool exists(const string& key) const;
	bool delete_(const string& key);

	void deleteAll();
	bool isEmpty() const;
	uint getSize() const;
	array<string>& getKeys() const;

	value& operator[](const string& key);
	const value& operator[](const string& key)const;

	assume iter begin()const;
	assume iter end()const;
	assume dictionary(std::initializer_list<std::pair<string, value>>);
};

struct datetime
{
	datetime();
	datetime(const datetime&);
	datetime(uint64 TimeStamp); //扩展 接受时间戳
	datetime(uint y, uint m, uint d, uint h = 0, uint mi = 0, uint s = 0);

	uint get_year() const;
	uint get_month() const;
	uint get_day() const;
	uint get_hour() const;
	uint get_minute() const;
	uint get_second() const;
	bool setDate(uint year, uint month, uint day);
	bool setTime(uint hour, uint minute, uint second);
	uint64 toTimeStamp(); //扩展 转时间戳

	datetime& operator= (const datetime&);
	datetime& operator() (const datetime&);
	datetime& operator- (const datetime&);
	datetime& operator+ (const datetime&);
	datetime& operator-= (const datetime&);
	datetime& operator+= (const datetime&);
	bool operator== (const datetime&) const;
	bool operator!= (const datetime&) const;
	bool operator>= (const datetime&) const;
	bool operator<= (const datetime&) const;
	bool operator< (const datetime&) const;
	bool operator> (const datetime&) const;
	bool operator== (const int) const;
	bool operator!= (const int) const;
	bool operator>= (const int) const;
	bool operator<= (const int) const;
	bool operator< (const int) const;
	bool operator> (const int) const;
	bool operator== (const uint) const;
	bool operator!= (const uint) const;
	bool operator>= (const uint) const;
	bool operator<= (const uint) const;
	bool operator< (const uint) const;
	bool operator> (const uint) const;

	assume null_t operator=(null_t) = delete;
};

struct file
{
	file();
	//file(const file&) = delete;
	file& operator=(const file&) = delete;
	int open(const string& filename, const string& mode);
	int close();
	int getSize() const;
	bool isEndOfFile() const;
	string readString(uint length);
	string readLine();
	int64 readInt(uint bytes);
	uint64 readUInt(uint bytes);
	float readFloat();
	double readDouble();
	int writeString(const string& str);
	int writeInt(int64 value, uint bytes);
	int writeUInt(uint64 value, uint bytes);
	int writeFloat(float value);
	int writeDouble(double value);
	int getPos() const;
	int setPos(int pos);
	int movePos(int delta);

	bool mostSignificantByteFirst;
};

struct filesystem
{
	filesystem();
	//filesystem(const filesystem&) = delete;
	filesystem& operator=(const filesystem&) = delete;
	bool changeCurrentPath(const string path);
	string getCurrentPath() const;
	array<string>& getDirs() const;
	array<string>& getFiles() const;
	bool isDir(const string) const;
	int makeDir(const string);
	int removeDir(const string);
	int deleteFile(const string);
	int copyFile(const string, const string);
	int move(const string, const string);
	datetime getCreateDateTime(const string) const;
	datetime getModifyDateTime(const string) const;
};

template<typename T>
struct grid
{
	grid();
	//grid(const grid&) = delete;
	grid& operator=(const grid&) = delete;
	grid(uint, uint);
	grid(uint, uint, const T&);

	T& operator[](uint i, uint j);
	const T& operator[](uint i, uint j) const;

	void resize(uint width, uint height);
	uint width() const;
	uint height() const;
};

struct ref
{
	ref();

	template<typename type>
	ref(const type&);

	template<typename type>
	explicit operator type() const;

	//仅支持$(句柄)赋值
	template<typename type>
	ref& operator=(const type&);

	template<typename type>
	bool operator==(const type&);
};

//仅支持脚本代码定义类型，系统内置类型报错
template<typename T>
struct weakref
{
	weakref();
	weakref(const weakref&);
	weakref(const T&);

	T& get()const;

	explicit operator T() const;

	weakref<T>& operator=(const T&);

	bool operator==(const T&);
};

struct any
{
	any();

	template<typename type>
	void store(const type& in);
	template<typename type>
	bool retrieve(type& out) const;

	template<typename type>
	any(const type&);

	template<typename type>
	explicit operator type() const;

	template<typename type>
	any& operator=(const type&);
};

struct socket
{
	socket();
	//socket(const socket&) = delete;
	socket& operator=(const socket&) = delete;
	int listen(uint16 port);
	int close();
	socket& accept(int64 timeout = 0);
	int connect(uint ipv4address, uint16 port);
	int send(const string& data);
	string receive(int64 timeout = 0);
	bool isActive() const;
};

assume struct null_t
{
	null_t();

	template<typename type>
	operator type() const;

	template<>
	operator string() const = delete;

	template<>
	operator complex() const = delete;

	template<>
	operator datetime() const = delete;
};

////////////////////////////////

string operator+(const char* lhs, const string& rhs);
string operator+(int8 lhs, const string& rhs);
string operator+(uint8 lhs, const string& rhs);
string operator+(int16 lhs, const string& rhs);
string operator+(uint16 lhs, const string& rhs);
string operator+(int lhs, const string& rhs);
string operator+(uint lhs, const string& rhs);
string operator+(int64 lhs, const string& rhs);
string operator+(uint64 lhs, const string& rhs);
string operator+(float lhs, const string& rhs);
string operator+(double lhs, const string& rhs);

string operator""s(const char* str, size_t len);

////////////////////////////////
#pragma endregion

//排除不支持引用的类型
#define TypeExclude(t)  \
template<>\
assume bool operator==<t>(const t&, const null_t&)  = delete;\
template<>\
assume bool operator==<t>(const null_t&, const t&)  = delete;\
template<>\
assume bool operator!=<t>(const t&, const null_t&)  = delete;\
template<>\
assume bool operator!=<t>(const null_t&, const t&)  = delete;\

template<typename type>
assume bool operator==(const type&, const null_t&);
template<typename type>
assume bool operator==(const null_t&, const type&);
template<typename type>
assume bool operator!=(const type&, const null_t&);
template<typename type>
assume bool operator!=(const null_t&, const type&);

TypeExclude(dictionary::value);
TypeExclude(string);
TypeExclude(complex);
TypeExclude(datetime);

///////////////////////////////////////////////////////////////////////////////////////////

//为了兼容C++代码编辑器识别AngelScript代码，故作以下宏定义，不会造成实质上的宏替换

//输入，示例 &in（引入）
#define in

//输出，示例 &out（引出）
#define out

//输入输出，示例 &inout（引用）
#define inout

//对象句柄，为了兼容C++代码编辑器识别，$ 代替 @
//$ a = $ b (可以简写为 $ a = b) 这是对句柄变量本身赋值; a = b 这是对句柄变量引用的对象赋值 
//$ a == $ b 这是对句柄变量本身比较; a == b 这是对句柄变量引用的对象比较 
#define $

//函数定义，为了兼容C++代码编辑器识别，应该在funcdef 语句的下一行插入 虚指令 functyp(f) 示例: 
//funcdef int add(int a, int b);
//functyp(add);
//int example(int a, int b, add$ func) { return func(a, b); }
//该关键字不能出现在函数代码块里
#define funcdef

//函数类型，虚指令 用途:兼容C++代码编辑器识别
#define functyp(f) using f##$ = std::Function<decltype(f)>

//lambda(匿名函数)
//赋值语句示例 $ fun = function(){};
#define function [=]

//空的对象句柄变量
#define null null_t()

//接口类 只能定义声明，不能实现
#define interface struct

//普通类 成员访问默认即为public
#define class struct

//混入类
#define mixin

//抽象类 不能实例化，只有继承的子类才能实例化
#define abstract

//最终的 修饰类=不可继承，修饰函数=不可覆盖
#define final

// 类型转换
// 不可 cast<类型>() = item，string，complex，datetime，int ...
// 因为这些类型不支持对象句柄引用操作
#define cast static_cast

//调用父类构造函数
#define super(...)

//对象自身
#define this *this

//私有
#define private private:

//保护
#define protected protected:

#if _MSVC_LANG  < 202002L
//导入
#define import

//来自
#define from {};string s =
#endif

//共享的
#define shared

//外部的
#define external

//遍历 示例 foreach(auto elem : list)
#define foreach for

//是，用于句柄变量自身判断
#define is ==

//元数据，预处理指令 用途：初始化对象转格式化字符串
//注意!!! 代码体 可以出现 宏替换，不可出现枚举常量，因为这是预处理字符串
#define metadata std::Function<void(void)>() = []()

//逻辑与
#define and &&
//逻辑或
#define or ||
//逻辑异或
#define xor ^
