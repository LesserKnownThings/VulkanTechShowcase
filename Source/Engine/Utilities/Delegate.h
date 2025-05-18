#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

#define FUNC_CONCAT(...) __VA_ARGS__

#define FUNC_BIND_DELEGATE(name, ...) \
	typedef Delegate<__VA_ARGS__> name;

#define DECLARE_DELEGATE(name) FUNC_BIND_DELEGATE(name)
#define DECLARE_DELEGATE_OneParam(name, param_1) FUNC_BIND_DELEGATE(name, param_1)
#define DECLARE_DELEGATE_TwoParams(name, param_1, param_2) FUNC_BIND_DELEGATE(name, param_1, param_2)
#define DECLARE_DELEGATE_ThreeParams(name, param_1, param_2, param_3) FUNC_BIND_DELEGATE(name, param_1, param_2, param_3)

template <typename... Args>
class Delegate
{
	typedef void* ptr;
	using functionPtr = std::function<void(Args...)>;

	template <class C>
	using memberFunctionPtr = void (C::*)(Args...);

public:
	bool IsBound() const
	{
		return internalStubs.size() > 0;
	}

	template <class C>
	void Bind(C* owningObj, memberFunctionPtr<C> funcPtr)
	{
		functionPtr new_func_ptr =
			[owningObj, funcPtr](Args... args)
			{
				(owningObj->*funcPtr)(args...);
			};

		internalStubs.insert({ owningObj, new_func_ptr });
	}

	void Invoke(Args... args) const
	{
		for (const std::pair<ptr, functionPtr>& func : internalStubs)
		{
			func.second(args...);
		}
	}

	template <class C>
	void Clear(const C* instance)
	{
		internalStubs.erase((void*)instance);
	}

private:
	std::unordered_multimap<ptr, functionPtr> internalStubs;
};
