/*
 * Copyright 2020 Redpanda Data, Inc.
 *
 * Use of this software is governed by the Business Source License
 * included in the file licenses/BSL.md
 *
 * As of the Change Date specified in that file, in accordance with
 * the Business Source License, use of this software will be governed
 * by the Apache License, Version 2.0
 */

#pragma once
#include "config/validation_error.h"
#include "json/stringbuffer.h"
#include "json/writer.h"
#include "seastarx.h"

#include <seastar/util/bool_class.hh>

#include <yaml-cpp/yaml.h>

#include <any>
#include <iosfwd>
#include <string>

namespace config {

// String to use when logging the value of a secret property
static constexpr std::string_view secret_placeholder = "[secret]";

class config_store;
using required_t = ss::bool_class<struct required_tag>;
using needs_restart_t = ss::bool_class<struct needs_restart_tag>;
using is_secret = ss::bool_class<struct is_secret_tag>;

// Whether to redact secrets. If true, `secret_placeholder` should be used
// instead of the config value.
using redact_secrets = ss::bool_class<struct redact_secrets_tag>;

enum class visibility_t {
    // Tunables can be set by the user, but they control implementation
    // details like (e.g. buffer sizes, queue lengths)
    tunable,
    // User properties are normal, end-user visible settings that control
    // functional redpanda behaviours (e.g. enable a feature)
    user,
    // Deprecated properties are kept around to avoid complaining
    // about invalid config after upgrades, but they do nothing and
    // should never be presented to the user for editing.
    deprecated,
};

std::string_view to_string_view(visibility_t v);

class base_property {
public:
    struct metadata {
        required_t required{required_t::no};
        needs_restart_t needs_restart{needs_restart_t::yes};
        std::optional<ss::sstring> example{std::nullopt};
        visibility_t visibility{visibility_t::user};
        is_secret secret{is_secret::no};
    };

    base_property(
      config_store& conf,
      std::string_view name,
      std::string_view desc,
      metadata meta);

    const std::string_view& name() const { return _name; }
    const std::string_view& desc() const { return _desc; }

    const required_t is_required() const { return _meta.required; }
    bool needs_restart() const { return bool(_meta.needs_restart); }
    visibility_t get_visibility() const { return _meta.visibility; }
    bool is_secret() const { return bool(_meta.secret); }

    // this serializes the property value. a full configuration serialization is
    // performed in config_store::to_json where the json object key is taken
    // from the property name.
    virtual void to_json(
      json::Writer<json::StringBuffer>& w, redact_secrets redact) const = 0;

    virtual void print(std::ostream&) const = 0;
    virtual bool set_value(YAML::Node) = 0;
    virtual void set_value(std::any) = 0;
    virtual void reset() = 0;
    virtual bool is_default() const = 0;

    /**
     * Helper for logging string-ized values of a property, e.g.
     * while processing an API request or loading from file, before
     * the property itself is initialized.
     *
     * Use this to ensure that any logged values are properly
     * redacted if secret.
     */
    template<typename U>
    std::string_view format_raw(U const& in) {
        if (is_secret() && !in.empty()) {
            return secret_placeholder;
        } else {
            return in;
        }
    }

    virtual std::string_view type_name() const = 0;
    virtual std::optional<std::string_view> units_name() const = 0;
    virtual bool is_nullable() const = 0;
    virtual bool is_array() const = 0;
    virtual std::optional<std::string_view> example() const = 0;
    virtual std::vector<ss::sstring> enum_values() const { return {}; };

    /**
     * Validation of a proposed new value before it has been assigned
     * to this property.
     */
    virtual std::optional<validation_error> validate(YAML::Node) const = 0;
    virtual base_property& operator=(const base_property&) = 0;
    virtual ~base_property() noexcept = default;

private:
    friend std::ostream& operator<<(std::ostream&, const base_property&);
    std::string_view _name;
    std::string_view _desc;

protected:
    metadata _meta;
    void assert_live_settable() const;
};
}; // namespace config
