#include <type_traits>;

namespace std
{
	template <class _Tx>
	struct _Get_function_impl {
	};

	template <class _Ret, class... _Types>
	class _Func_class : public _Arg_types<_Types...> {
	public:
		_Ret operator()(_Types... _Args) const;

	protected:
		template <class _Fx, class _Function>
		using _Enable_if_callable_t = enable_if_t<conjunction_v<negation<is_same<_Remove_cvref_t<_Fx>, _Function>>,
			_Is_invocable_r<_Ret, decay_t<_Fx>&, _Types...>>,
			int>;
	};

#define _GET_FUNCTION_IMPL(CALL_OPT, X1, X2, X3)                                                  \
    template <class _Ret, class... _Types>                                                        \
    struct _Get_function_impl<_Ret CALL_OPT(_Types...)> { /* determine type from argument list */ \
        using type = _Func_class<_Ret, _Types...>;                                                \
    };

	_NON_MEMBER_CALL(_GET_FUNCTION_IMPL, X1, X2, X3)
#undef _GET_FUNCTION_IMPL;

}

namespace std
{
	template <class _Fty>
	class Function;

	template <class _Fty>
	class Function : public _Get_function_impl<_Fty>::type { // wrapper for callable objects
	private:
		using _Mybase = typename _Get_function_impl<_Fty>::type;

	public:
		Function() noexcept {}

		Function(nullptr_t) noexcept {}

		template <class _Fx, typename _Mybase::template _Enable_if_callable_t<_Fx, Function> = 0>
		Function(_Fx&& _Func);

		template <class _Fx, typename _Mybase::template _Enable_if_callable_t<_Fx, Function> = 0>
		Function& operator=(_Fx&& _Func);

		Function& operator=(nullptr_t) noexcept;
	};

	template <typename _Elem>
	struct initializer_list {
	public:
		initializer_list(const _Elem*, const _Elem*);

		const _Elem* begin() const;

		const _Elem* end() const;

		size_t size() const;
	};

	template <typename _Ty1, typename _Ty2>
	struct pair {
		pair(_Ty1, _Ty2);
	};
}
