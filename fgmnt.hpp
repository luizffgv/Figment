/**
 * @file fgmnt.hpp
 * @author Luiz Fernando F. G. Valle (luizfvalle@pm.me)
 * @brief Introduces the figment class and the fgmnt namespace
 */

/*! \mainpage Figment
 *
 * \section intro_sec What is this?
 *
 * Figment allows you to control precisely the lifetime of objects of any type.
 * The \ref fgmnt::figment class is similar to std::optional, but it holds no
 * information about itself.
 *
 * \subsection name Why the name?
 *
 * > Figments are subjective in that they are based on personal experiences and
 * > perceptions and may not be considered objective or verifiable by others.
 * -- <cite>Text generated by GPT-3.5—OpenAI’s large-scale language-generation
 * model—and adapted by me</cite>
 *
 * The basis of Figment is that it's sometimes possible to deduce the state of
 * an object without verifying it. A \link fgmnt::figment figment \endlink
 * shouldn't be used alongside boolean flags. Any code that can access a \link
 * fgmnt::figment figment \endlink should be unreachable under any unsafe
 * circumstances. Take for example a class instance: its member variables will
 * never be accessed again once the constructor throws. I bet you can come up
 * with other use cases.
 *
 * \subsection safety Is this safe? Like, at all?
 *
 * As long as you don't make any mistakes. So **definitely not!** A \link
 * fgmnt::figment figment \endlink is fundamentally unsafe and instances should
 * be used with care, and even then it's very easy to fuck everything up. Don't
 * use this library if value your sanity over saving a few bytes.
 *
 * \section Usage
 *
 * Check the class documentation \link fgmnt::figment here \endlink.
 */

#pragma once

#include <concepts>         // constructible_from
#include <initializer_list> // initializer_list
#include <iterator>         // begin, end
#include <memory>           // addressof
#include <new>              // launder, placement new
#include <type_traits>      // is_reference_v, is_volatile_v
#include <utility>          // as_const, forward

// TODO Properly detect C++23 for CONSTEXPR_IF_CPP23 when we have the
// __cplusplus value

// clang-format off
#if __cpp_if_consteval >= 202106L
#define IF_CONSTEVAL(on_true, on_false) \
if consteval { on_true; } else { on_false; }
#define CONSTEXPR_IF_CPP23 constexpr
#else
#define CONSTEXPR_IF_CPP23
#define IF_CONSTEVAL(on_true, on_false) \
do { on_false; } while (false)
#endif
// clang-format on

#if __cplusplus >= 202002L
#    define CONSTEXPR_IF_CPP20 constexpr
#else
#    define CONSTEXPR_IF_CPP20
#endif

#define OPERATOR(op)                                                        \
    template <typename Arg>                                                 \
    decltype(auto) operator op(Arg &&rhs)                                   \
    {                                                                       \
        return launder_if_const_and_cpp17(value) op std::forward<Arg>(rhs); \
    }

#define OPERATOR_CONST(op)                                      \
    template <typename Arg>                                     \
    decltype(auto) operator op(Arg &&rhs) const                 \
    {                                                           \
        return std::as_const(launder_if_const_and_cpp17(value)) \
          op std::forward<Arg>(rhs);                            \
    }

namespace fgmnt
{
/**
 * @brief A figment holds either an object or nothing.
 *
 * Holds no information whatsoever about whether or not an object is
 * currently stored. Only use this class if the accessor code is unreachable
 * under unsafe conditions. Managing the underlying objects' lifetime is
 * your responsibility.
 *
 * If you need a boolean flag to track the state of a figment, this isn't
 * for you. Use an std::optional instead.
 *
 * There's not much point in using a const figment. You can use a const T, or
 * temporarily call T's const member functions using std::as_const on the
 * figment.
 *
 * @tparam T Type of the underlying object
 */
template <typename T>
#ifdef __cpp_concepts
requires(!std::is_reference_v<T> && !std::is_volatile_v<T>)
#endif
  union figment
{
    static constexpr T &launder_if_const_and_cpp17(T &obj)
    {
        if constexpr (std::is_const_v<T> && __cplusplus <= 201703L)
            return *std::launder(&obj);
        else
            return obj;
    }

public:
    /// Underlying object type
    using value_type = T;

    /// Does nothing. You have to initialize the underlying object manually.
    CONSTEXPR_IF_CPP20 figment() {}

    /// Does nothing. You have to destroy the underlying object manually.
    CONSTEXPR_IF_CPP20 ~figment() {}

    /**
     * @brief Starts the lifetime of the underlying object
     *
     * @param args Arguments to initialize the object with, as if by calling
     * T's constructor.
     */
    template <typename... Args>
#if __cpp_lib_concepts >= 202002L
    requires std::constructible_from<T, Args...>
#endif
      CONSTEXPR_IF_CPP20 figment(Args &&...args)
        : value(std::forward<Args>(args)...)
    {
    }

    /**
     * @brief Starts the lifetime of the underlying object
     *
     * @param list Initializer list to initialize the object with, as if by
     *             calling the constructor T::T(\ref std::initializer_list).
     */
    template <typename U>
#if __cpp_lib_concepts >= 202002L
    requires std::constructible_from<T, std::initializer_list<U>>
#endif
      CONSTEXPR_IF_CPP20 explicit figment(std::initializer_list<U> list)
        : value{list}
    {
    }

    /**
     * @brief Starts the lifetime of the underlying object
     *
     * The behavior is undefined if an object already exists. End its
     * lifetime with destroy().
     *
     * @param args Arguments to initialize the object with, as if by calling
     * T's constructor.
     */
    template <typename... Args>
#if __cpp_lib_concepts >= 202002L
    requires std::constructible_from<T, Args...>
#endif
      CONSTEXPR_IF_CPP23 void create(Args &&...args)
    {
        IF_CONSTEVAL((std::construct_at(&value, std::forward<Args>(args)...)),
                     (new (this) T{std::forward<Args>(args)...}));
    }

    /**
     * @brief Starts the lifetime of the underlying object
     *
     * The behavior is undefined if an object already exists. End its
     * lifetime with destroy().
     *
     * @param list Initializer list to initialize the object with, as if by
     *             calling the constructor T::T(\ref std::initializer_list).
     */
    template <typename U>
#if __cpp_lib_concepts >= 202002L
    requires std::constructible_from<T, std::initializer_list<U>>
#endif
      CONSTEXPR_IF_CPP23 void create(std::initializer_list<U> list)
    {
        IF_CONSTEVAL((std::construct_at(&value, list)), (new (this) T{list}));
    }

    /// Destroys the underlying object, ending its lifetime.
    CONSTEXPR_IF_CPP20 void destroy()
    {
        launder_if_const_and_cpp17(value).~T();
    }

    /**
     * @brief Replaces the underlying object
     *
     * Calls T's destructor on the object, and creates another object as if
     * using create().
     *
     * The behavior is undefined if there's no underlying object
     *
     * @param args Arguments to initialize the new object with, as if by
     * calling T's constructor.
     */
    template <typename... Args>
#if __cpp_lib_concepts >= 202002L
    requires std::constructible_from<T, Args...>
#endif
      CONSTEXPR_IF_CPP20 void replace(Args &&...args)
    {
        destroy();
        create(std::forward<Args>(args)...);
    }

    /**
     * @brief Replaces the underlying object
     *
     * Calls T's destructor on the object, and creates another object as if
     * using create().
     *
     * The behavior is undefined if there's no underlying object
     *
     * @param list Initializer list to initialize the new object with, as if
     * by calling the constructor T::T(\ref std::initializer_list).
     */
    template <typename U>
#if __cpp_lib_concepts >= 202002L
    requires std::constructible_from<T, std::initializer_list<U>>
#endif
      CONSTEXPR_IF_CPP20 void replace(std::initializer_list<U> list)
    {
        destroy();
        create(list);
    }

    /// Accessor to the underlying object
    CONSTEXPR_IF_CPP20 T &to_underlying()
    {
        return launder_if_const_and_cpp17(value);
    }

    /**
     * @brief Accessor to the underlying object, as if it were of type \ref
     * std::add_const_t<T>.
     */
    CONSTEXPR_IF_CPP20 const T &to_underlying() const
    {
        return std::as_const(launder_if_const_and_cpp17(value));
    }

    /// Same as calling std::begin with the underlying object
    CONSTEXPR_IF_CPP20 decltype(auto) begin()
#ifdef __cpp_concepts
      requires requires(T obj)
    {
        std::begin(obj);
    }
#endif
    {
        return std::begin(launder_if_const_and_cpp17(value));
    }

    /**
     * @brief Same as calling std::begin with the underlying object, as if it
     * were of type \ref std::add_const_t<T>.
     */
    CONSTEXPR_IF_CPP20 decltype(auto) begin() const
#ifdef __cpp_concepts
      requires requires(const T obj)
    {
        std::begin(obj);
    }
#endif
    {
        return std::begin(std::as_const(launder_if_const_and_cpp17(value)));
    }

    /// Same as calling std::end with the underlying object
    CONSTEXPR_IF_CPP20 decltype(auto) end()
#ifdef __cpp_concepts
      requires requires(T obj)
    {
        std::end(obj);
    }
#endif
    {
        return std::end(launder_if_const_and_cpp17(value));
    }

    /**
     * @brief Same as calling std::end with the underlying object, as if it were
     * of type \ref std::add_const_t<T>.
     */
    CONSTEXPR_IF_CPP20 decltype(auto) end() const
#ifdef __cpp_concepts
      requires requires(const T obj)
    {
        std::end(obj);
    }
#endif
    {
        return std::end(std::as_const(launder_if_const_and_cpp17(value)));
    }

    /// Accesses the members of the underlying object
    CONSTEXPR_IF_CPP20 T *operator->()
    {
        return &launder_if_const_and_cpp17(value);
    }

    /**
     * @brief Accesses the members of the underlying object, as if it were
     * of type \ref std::add_const_t<T>.
     */
    CONSTEXPR_IF_CPP20 const T *operator->() const
    {
        return &std::as_const(launder_if_const_and_cpp17(value));
    }

    /// Same as using the & operator in the underlying object
    CONSTEXPR_IF_CPP20 decltype(auto) operator&()
    {
        return &launder_if_const_and_cpp17(value);
    }

    /**
     * @brief Same as using the & operator in the underlying object, as if
     * it were of type \ref std::add_const_t<T>.
     */
    CONSTEXPR_IF_CPP20 decltype(auto) operator&() const
    {
        return &std::as_const(launder_if_const_and_cpp17(value));
    }

    /// Same as using the [] operator in the underlying object
    template <typename Arg>
    CONSTEXPR_IF_CPP20 decltype(auto) operator[](Arg &&rhs)
    {
        return launder_if_const_and_cpp17(value)[std::forward<Arg>(rhs)];
    }

    /**
     * @brief Same as using the [] operator in the underlying object, as if
     * it were of type \ref std::add_const_t<T>.
     */
    template <typename Arg>
    CONSTEXPR_IF_CPP20 decltype(auto) operator[](Arg &&rhs) const
    {
        return std::as_const(
          launder_if_const_and_cpp17(value))[std::forward<Arg>(rhs)];
    }

    /// Same as using the = operator in the underlying object
    template <typename Arg>
    CONSTEXPR_IF_CPP20 decltype(auto) operator=(Arg &&rhs)
    {
        return launder_if_const_and_cpp17(value) = std::forward<Arg>(rhs);
    }

    /**
     * @brief Same as using the = operator in the underlying object, as if
     * it were of type \ref std::add_const_t<T>.
     */
    template <typename Arg>
    CONSTEXPR_IF_CPP20 decltype(auto) operator=(Arg &&rhs) const
    {
        return std::as_const(launder_if_const_and_cpp17(value))
               = std::forward<Arg>(rhs);
    }

#if __cplusplus >= 202002L
    /// Same as using the <=> operator in the underlying object
    template <typename Arg>
    CONSTEXPR_IF_CPP20 decltype(auto) operator<=>(Arg &&rhs)
    {
        return launder_if_const_and_cpp17(value) <=> std::forward<Arg>(rhs);
    }

    /**
     * @brief Same as using the <=> operator in the underlying object, as if
     * it were of type \ref std::add_const_t<T>.
     */
    template <typename Arg>
    CONSTEXPR_IF_CPP20 decltype(auto) operator<=>(Arg &&rhs) const
    {
        return std::as_const(launder_if_const_and_cpp17(value))
               <=> std::forward<Arg>(rhs);
    }
#else
    /// Same as using the < operator in the underlying object
    OPERATOR(<)

    /**
     * @brief Same as using the < operator in the underlying object, as if
     * it were of type \ref std::add_const_t<T>.
     */
    OPERATOR_CONST(<)

    /// Same as using the <= operator in the underlying object
    OPERATOR(<=)

    /**
     * @brief Same as using the <= operator in the underlying object, as if
     * it were of type \ref std::add_const_t<T>.
     */
    OPERATOR_CONST(<=)

    /// Same as using the > operator in the underlying object
    OPERATOR(>)

    /**
     * @brief Same as using the > operator in the underlying object, as if
     * it were of type \ref std::add_const_t<T>.
     */
    OPERATOR_CONST(>)

    /// Same as using the >= operator in the underlying object
    OPERATOR(>=)

    /**
     * @brief Same as using the >= operator in the underlying object, as if
     * it were of type \ref std::add_const_t<T>.
     */
    OPERATOR_CONST(>=)
#endif

    /// Same as using the == operator in the underlying object
    template <typename Arg>
    CONSTEXPR_IF_CPP20 bool operator==(Arg &&rhs)
    {
        return launder_if_const_and_cpp17(value) == std::forward<Arg>(rhs);
    }

    /**
     * @brief Same as using the == operator in the underlying object, as if
     * it were of type \ref std::add_const_t<T>.
     */
    template <typename Arg>
    CONSTEXPR_IF_CPP20 bool operator==(Arg &&rhs) const
    {
        return std::as_const(launder_if_const_and_cpp17(value))
               == std::forward<Arg>(rhs);
    }

    // NOTE N4868 [class.conv.fct]
    // A conversion function is never used to convert a (possibly
    // cv-qualified) object to the (possibly cv-qualified) same object type
    // (or a reference to it). So we can make our own without overriding any
    // functionality of T.
    //
    // So there's no harm done in overriding T's T conversion function.
    //
    // Technically, such conversion functions of T can be called manually,
    // or be potentially reached through a call to a virtual conversion
    // function in a base class, but who gives a fuck? Figment isn't made to
    // babysit your cursed types.

    /// Returns a reference to the underlying object
    CONSTEXPR_IF_CPP20 operator T &()
    {
        return launder_if_const_and_cpp17(value);
    }

    /// Returns a const reference to the underlying object
    CONSTEXPR_IF_CPP20 operator const T &() const
    {
        return launder_if_const_and_cpp17(value);
    }

private:
    T value;
};

} // namespace fgmnt

#undef IF_CONSTEVAL
#undef CONSTEXPR_IF_CPP23
#undef CONSTEXPR_IF_CPP20
#undef OPERATOR
#undef OPERATOR_CONST
