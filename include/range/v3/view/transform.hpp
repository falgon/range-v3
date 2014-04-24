// Boost.Range library
//
//  Copyright Thorsten Ottosen, Neil Groves 2006 - 2008.
//  Copyright Eric Niebler 2013.
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org/libs/range/
//

#ifndef RANGES_V3_VIEW_TRANSFORM_HPP
#define RANGES_V3_VIEW_TRANSFORM_HPP

#include <utility>
#include <iterator>
#include <type_traits>
#include <range/v3/range_fwd.hpp>
#include <range/v3/size.hpp>
#include <range/v3/begin_end.hpp>
#include <range/v3/range_traits.hpp>
#include <range/v3/range_adaptor.hpp>
#include <range/v3/utility/bindable.hpp>
#include <range/v3/utility/invokable.hpp>
#include <range/v3/utility/optional.hpp>

namespace ranges
{
    inline namespace v3
    {
        namespace detail
        {
            template<typename UnaryFunction, typename SinglePass>
            struct transform_adaptor : default_adaptor
            {
            private:
                UnaryFunction const *fun_;
            public:
                using single_pass = SinglePass;
                transform_adaptor() = default;
                transform_adaptor(UnaryFunction const &fun)
                  : fun_(&fun)
                {}
                template<typename Cursor>
                auto current(Cursor const &pos) const ->
                    decltype((*fun_)(pos.current()))
                {
                    return (*fun_)(pos.current());
                }
            };
        }

        template<typename InputIterable, typename UnaryFunction>
        struct transformed_view
          : range_adaptor<transformed_view<InputIterable, UnaryFunction>, InputIterable>
        {
        private:
            using reference_t = result_of_t<invokable_t<UnaryFunction> const(range_value_t<InputIterable>)>;
            friend range_core_access;
            // TODO optional is only necessary here for non-SemiRegular types
            optional<invokable_t<UnaryFunction>> fun_;
            // Forward ranges must always return references. If the result of calling the function
            // is not a reference, this range is input-only.
            using single_pass = detail::or_t<
                Derived<ranges::input_iterator_tag, range_category_t<InputIterable>>,
                detail::not_t<std::is_reference<reference_t>>>;
            using adaptor_t = detail::transform_adaptor<invokable_t<UnaryFunction>, single_pass>;
            using use_sentinel_t = detail::or_t<detail::not_t<Range<InputIterable>>, single_pass>;
            adaptor_t begin_adaptor() const
            {
                RANGES_ASSERT(!!fun_);
                return {*fun_};
            }
            CONCEPT_REQUIRES(use_sentinel_t())
            default_adaptor end_adaptor() const
            {
                RANGES_ASSERT(!!fun_);
                return {};
            }
            CONCEPT_REQUIRES(!use_sentinel_t())
            adaptor_t end_adaptor() const
            {
                RANGES_ASSERT(!!fun_);
                return {*fun_};
            }
        public:
            transformed_view() = default;
            transformed_view(InputIterable && rng, UnaryFunction fun)
              : range_adaptor_t<transformed_view>(std::forward<InputIterable>(rng))
              , fun_(ranges::invokable(std::move(fun)))
            {}
            CONCEPT_REQUIRES(ranges::SizedIterable<InputIterable>())
            range_size_t<InputIterable> size() const
            {
                return this->base_size();
            }
        };

        namespace view
        {
            struct transformer : bindable<transformer>
            {
            private:
                template<typename UnaryFunction>
                struct transformer1 : pipeable<transformer1<UnaryFunction>>
                {
                private:
                    UnaryFunction fun_;
                public:
                    transformer1(UnaryFunction fun)
                      : fun_(std::move(fun))
                    {}
                    template<typename InputIterable, typename This>
                    static transformed_view<InputIterable, UnaryFunction>
                    pipe(InputIterable && rng, This && this_)
                    {
                        return {std::forward<InputIterable>(rng), std::forward<This>(this_).fun_};
                    }
                };
            public:
                ///
                template<typename InputIterable1, typename UnaryFunction>
                static transformed_view<InputIterable1, UnaryFunction>
                invoke(transformer, InputIterable1 && rng, UnaryFunction fun)
                {
                    CONCEPT_ASSERT(ranges::Iterable<InputIterable1>());
                    CONCEPT_ASSERT(ranges::InputIterator<range_iterator_t<InputIterable1>>());
                    CONCEPT_ASSERT(ranges::Invokable<UnaryFunction,
                                                     range_value_t<InputIterable1>>());
                    return {std::forward<InputIterable1>(rng), std::move(fun)};
                }

                /// \overload
                template<typename UnaryFunction>
                static transformer1<UnaryFunction> invoke(transformer, UnaryFunction fun)
                {
                    return {std::move(fun)};
                }
            };

            RANGES_CONSTEXPR transformer transform {};
        }
    }
}

#endif
