// Copyright 2021 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#pragma once

#include <cinttypes>
#include <concepts>
#include <type_traits>

namespace serde {

using version_t = uint8_t;

template<version_t V>
struct version {
    static constexpr auto const v = V;
};

template<version_t V>
struct compat_version {
    static constexpr auto const v = V;
};

/**
 * \brief provides versioning (version + compat version)
 *        for serializable aggregate types.
 *
 * \tparam Version         the current type version
 *                         (change for every incompatible update)
 * \tparam CompatVersion   the minimum required version able to parse the type
 */
template<
  typename T,
  typename Version,
  typename CompatVersion = compat_version<Version::v>>
struct envelope {
    bool operator==(envelope const&) const = default;
    auto operator<=>(envelope const&) const = default;
    using value_t = T;
    static constexpr auto redpanda_serde_version = Version::v;
    static constexpr auto redpanda_serde_compat_version = CompatVersion::v;
    static constexpr auto redpanda_inherits_from_envelope = true;
};

/**
 * Checksum envelope uses CRC32c to check data integrity.
 * The idea is that CRC32 has hardware support and is faster than
 * disk and network I/O. So it will not be a bottle neck.
 * This can be changed - for example by a separate template parameter
 * template <..., typename HashAlgo = crc32c>
 */
template<
  typename T,
  typename Version,
  typename CompatVersion = compat_version<Version::v>>
struct checksum_envelope {
    bool operator==(checksum_envelope const&) const = default;
    using value_t = T;
    static constexpr auto redpanda_serde_version = Version::v;
    static constexpr auto redpanda_serde_compat_version = CompatVersion::v;
    static constexpr auto redpanda_inherits_from_envelope = true;
    static constexpr auto redpanda_serde_build_checksum = true;
};

// clang-format off
namespace detail {

template<typename T>
concept has_compat_attribute = requires(T t) {
    t.redpanda_serde_compat_version;
};

template<typename T>
concept has_version_attribute = requires(T t) {
    t.redpanda_serde_version;
};

template<typename T>
concept compat_version_has_serde_version_type = requires(T t) {
    requires std::same_as<
      std::decay_t<decltype(t.redpanda_serde_compat_version)>,
      version_t>;
};

template<typename T>
concept version_has_serde_version_type = requires(T t) {
    requires std::same_as<
      std::decay_t<decltype(t.redpanda_serde_version)>,
      version_t>;
};

template<typename T>
concept has_checksum_attribute = requires(T t) {
    t.redpanda_serde_build_checksum;
};

} // namespace detail

template<typename T>
concept is_envelope =
  detail::has_compat_attribute<T>
  && detail::has_version_attribute<T>
  && detail::compat_version_has_serde_version_type<T>
  && detail::version_has_serde_version_type<T>;

template<typename T>
concept is_checksum_envelope =
  detail::has_compat_attribute<T>
  && detail::has_version_attribute<T>
  && detail::compat_version_has_serde_version_type<T>
  && detail::version_has_serde_version_type<T>
  && detail::has_checksum_attribute<T>;
// clang-format on

template<typename T>
concept inherits_from_envelope = requires(T t) {
    t.redpanda_inherits_from_envelope;
};

} // namespace serde
