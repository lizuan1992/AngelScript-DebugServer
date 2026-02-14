#include "system.hh"

funcdef int FuncAdd(int a, int b);
functyp(FuncAdd);

assume void as_output(const string& in);

int main()
{
	try
	{
		throw("throw test");
	}
	catch
	{
		auto str = getExceptionInfo();
		as_output(str);
	}

	FuncAdd$ funcMul;
	FuncAdd$ funcAdd = function(int a, int b)
	{
		auto c = a + b;
		c = a + b + 1;
		return c;
	};

	int a = funcAdd(5, 5);
	int b = 0;
	/*FuncAdd myfunc = function(int a, STRING b, int c)
	{

	};*/

	array<int> arr;
	for (int i = 0; i < 10; i++)
	{
		arr.insertLast(i);
	}

	dictionary dic;
	dic["abc"] = 5;
	dic["ddd"] = "hello";
	dic["fff"] = 3.14;
	dic["ggg"] = true;
	dic["arr"] = $ arr;
	int val = int(dic["abc"]);
	int acc = 0;

	dictionary dic2 =
	{
		{"dic1",dic},
		{"abcdef","fedcba"},
		{"number",5},
		{ "time" ,datetime()}
	};

	foreach(auto _var : dic)
	{
		_var;
	}

	return 0;
}
