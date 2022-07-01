// Copyright 2020 Redpanda Data, Inc.
//
// Use of this software is governed by the Business Source License
// included in the file licenses/BSL.md
//
// As of the Change Date specified in that file, in accordance with
// the Business Source License, use of this software will be governed
// by the Apache License, Version 2.0

#include "config/base_property.h"

#include "config/config_store.h"
#include "vassert.h"

#include <ostream>

namespace config {
base_property::base_property(
  config_store& conf,
  std::string_view name,
  std::string_view desc,
  base_property::metadata meta)
  : _name(name)
  , _desc(desc)
  , _meta(meta) {
    conf._properties.emplace(name, this);
}

std::ostream& operator<<(std::ostream& o, const base_property& p) {
    p.print(o);
    return o;
}

std::string_view to_string_view(visibility_t v) {
    switch (v) {
    case config::visibility_t::tunable:
        return "tunable";
    case config::visibility_t::user:
        return "user";
    case config::visibility_t::deprecated:
        return "deprecated";
    }

    return "{invalid}";
}

/**
 * Helper for property methods that should only be used
 * on live-settable properties.
 */
void base_property::assert_live_settable() const {
    vassert(
      _meta.needs_restart == needs_restart_t::no,
      "Property must be be marked as "
      "needs_restart_t::no");
}

}; // namespace config
