

#include <stddef.h>
#include <utility>
#include <limits>
#include <type_traits>
#include <algorithm>


namespace _impl {
template<template <typename> class Checker>
constexpr size_t find_index()
{
	return 0;
}

template<template <typename> class Checker, typename T, typename... Ts>
constexpr size_t find_index()
{
#if 0
	if (sizeof...(Ts) == 0)
		return 0;
#endif
	if (Checker<T>::value)
		return 0;
	return 1 + find_index<Checker, Ts...>();
}


template <typename Type, typename... Ts>
struct get_index;

template <typename Type, typename... Ts>
struct get_index<Type, Type, Ts...> : std::integral_constant<std::size_t, 0> {};

template <typename Type, typename Head, typename... Ts>
struct get_index<Type, Head, Ts...> : std::integral_constant<std::size_t, 1 + get_index<Type, Ts...>::value> {};
} // namespace _impl

template <typename U, typename... Ts>
constexpr size_t get_index = _impl::get_index<U, Ts...>::value;

template <template <typename> class Checker, typename... Ts>
constexpr size_t find_index = _impl::find_index<Checker, Ts...>();

#include <iostream>
#include <experimental/optional>


struct variant_shared {
	struct Destroy {
		using return_type = void;

		template<typename T>
			void operator()(T* data)
			{
				data->~T();
			}
	};
};

template <typename... Ts>
struct variant : variant_shared {
	static constexpr size_t size  = std::max({sizeof(Ts)...});
	static constexpr size_t align = std::max({alignof(Ts)...});

	using Storage = typename std::aligned_storage<size, align>::type;

	variant() = default;

	template<typename T>
	variant(T const& value)
	{
		set(value);
	}

	template<typename... Os>
	variant(variant<Os...> const& other)
	{
		if (other.index == variant<Os...>::invalid)
			return;

		index = other.apply(GetIndex{});
		other.apply(Copy{*this});

	}

	template<typename... Os>
	variant(variant<Os...>&& other)
	{
		if (other.index == variant<Os...>::invalid)
			return;

		index = other.apply(GetIndex{});
		other.apply(Move{*this});
		other.reset();
	}

	template<typename... Os>
	variant& operator=(variant<Os...> const& other)
	{
		if (other.index == variant<Os...>::invalid)
			return *this;

		index = other.apply(GetIndex{});
		other.apply(Copy{*this});
		return *this;
	}

	template<typename... Os>
	variant& operator=(variant<Os...>&& other)
	{
		if (other.index == variant<Os...>::invalid)
			return *this;

		other.apply(Move{*this});
		other.reset();
		return *this;
	}


	template<typename T>
	void set(T const& v)
	{
		reset();
		construct<T>(v);
		index = index_t(get_index<T, Ts...>);
	}

	template<typename T>
	bool try_set(T const& v)
	{
		if (check_type<T>()) {
			set(v);
			return true;
		}
		return false;
	}

	template<typename T>
	bool get(T& target) const
	{
		if (check_type<T>()) {
			target = *reinterpret_cast<T const*>(&storage);
			return true;
		}
		return false;
	}

	template<typename T>
	std::experimental::optional<T> get()
	{
		if (check_type<T>())
			return *reinterpret_cast<T const*>(&storage);

		return std::experimental::nullopt;
	}

	void reset()
	{
		if (index == invalid)
			return;
		destroy();
		index = invalid;
	}

	/*!
	 * This type is used to avoid accidental comparisons of
	 * indices from different variant types.
	 */
	enum class index_t : size_t { };
	static constexpr index_t invalid = index_t(std::numeric_limits<size_t>::max());

	template<typename T>
	bool check_type() const
	{
		return index == index_t(get_index<T, Ts...>);
	}

	index_t type_index() const
	{
		return index;
	}

private:
	/*!
	 * Different instantiations of variant should have access to
	 * eachother's internals, as though they are of same class.
	 */
	template<typename... Os>
	friend class variant;

	// Helpers
	template<typename T, typename... Args>
	void construct(Args&&... args)
	{
		new (&storage) T(std::forward<Args>(args)...);
	}

	void destroy()
	{
		apply(Destroy{});
	}

	// Functors
	using variant_shared::Destroy;

	struct GetIndex {
		using return_type = index_t;

		template<typename T>
		index_t operator()(T const*)
		{
			return index_t(get_index<T,Ts...>);
		}
	};

	struct Copy {
		using return_type = void;

		Copy(variant& self)
			: self(self)
		{}

		template<typename T>
		void operator()(T const* value)
		{
			self.construct<T>(*value);
		}

	private:
		variant& self;
	};

	struct Move {
		using return_type = void;

		Move(variant& self)
			: self(self)
		{}

		template<typename T>
		void operator()(T* value)
		{
			self.construct<T>(std::move(*value));
		}

	private:
		variant& self;
	};

	// Functor dispatch
	template<typename Functor, typename T, typename... Args> static auto
	apply_functor(void* storage, Functor f, Args&&...args) -> typename Functor::return_type
	{
		return f(reinterpret_cast<T*>(storage), std::forward<Args>(args)...);
	}

	template<typename Functor, typename T, typename... Args> static auto
	apply_functor(void const* storage, Functor f, Args&&...args) -> typename Functor::return_type
	{
		return f(reinterpret_cast<T const*>(storage), std::forward<Args>(args)...);
	}

	template<typename Functor, typename...Args>
	auto apply(Functor f, Args&&... args) -> typename Functor::return_type
	{
		using return_type = typename Functor::return_type;
		using func_type   = return_type(void* storage, Functor f, Args...);

		static func_type* table[sizeof...(Ts)] = {
			(apply_functor<Functor, Ts, Args...>)...
		};

		return table[(size_t)index]((void*)&storage, f, std::forward<Args>(args)...);
	}

	template<typename Functor, typename...Args>
	auto apply(Functor f, Args&&... args) const -> typename Functor::return_type
	{
		using return_type = typename Functor::return_type;
		using func_type   = return_type(void const* storage, Functor f, Args...);

		static func_type* table[sizeof...(Ts)] = {
			(apply_functor<Functor, Ts, Args...>)...
		};

		return table[(size_t)index]((void*)&storage, f, std::forward<Args>(args)...);
	}

	// Storage
	Storage storage;
	index_t index = invalid;
};

template<typename T>
struct ISCONST {
	static constexpr bool value = !std::is_const<T>::value;
};

template<typename EC>
auto to_underlying(EC ec) -> typename std::underlying_type<EC>::type
{
	return typename std::underlying_type<EC>::type(ec);
}

int main()
{
	variant<int, float, std::string> var1(std::string("222"));
	variant<float, int, std::string> var2;
	var2 = var1;

	std::cout << std::boolalpha << var1.check_type<int>() << " " << var1.check_type<std::string>() << "\n";

	std::string s;
	var1.get(s);
	std::cout << s << "\n";
	var2.get(s);
	std::cout << s << "\n";

	auto i = var1.get<int>();
	if (i)
		std::cout << "int\n";
	else
		std::cout << "not int\n";

	auto size = find_index<ISCONST, const int, const float, const int>;
	std::cout << size << "\n";

	var1.set(std::string("555"));

	variant<int, float, std::string> var3;
	var3 = std::move(var1);

	std::cout << to_underlying(var3.type_index()) << "\n";
	std::cout << *var3.get<std::string>() << "\n";
	std::cout << to_underlying(var1.type_index()) << "\n";
}
