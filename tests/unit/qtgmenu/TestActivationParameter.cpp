/*
 * Copyright (C) 2026 Ubuntu Unity Life Support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// QtGActionGroup::ActivationParameter (UNITY-20261009-004): one test per row of
// the decision table, and the target types the survey found in GTK
// applications (s, b, x, (uu), (ss)). No D-Bus: the function is pure.

#include <libqtgmenu/internal/QtGActionGroup.h>

#include <gtest/gtest.h>
#include <gio/gio.h>

using namespace qtgmenu;

namespace {

// Prints the activation's parameter with its type, or "NULL"; unrefs it.
// A parameter that activates must not be floating: the caller unrefs it after
// g_action_group_activate_action, which would otherwise have taken it over.
std::string take(QtGActionGroup::Activation activation) {
	if (!activation.activate) {
		EXPECT_EQ(nullptr, activation.parameter);
		return "no activation";
	}
	if (activation.parameter == nullptr) {
		return "NULL";
	}
	EXPECT_FALSE(g_variant_is_floating(activation.parameter));
	gchar *printed = g_variant_print(activation.parameter, TRUE);
	std::string result(printed);
	g_free(printed);
	g_variant_unref(activation.parameter);
	return result;
}

std::string run(bool known, const char *type, const char *target) {
	return take(QtGActionGroup::ActivationParameter(known,
			type ? G_VARIANT_TYPE(type) : nullptr,
			target ? QString::fromUtf8(target) : QString(), "the-action"));
}

TEST(TestActivationParameter, UnknownActionDoesNotRun) {
	EXPECT_EQ("no activation", run(false, nullptr, nullptr));
	EXPECT_EQ("no activation", run(false, "s", "'list'"));
}

TEST(TestActivationParameter, NoTargetNoParameterRunsWithNull) {
	EXPECT_EQ("NULL", run(true, nullptr, nullptr));
}

TEST(TestActivationParameter, NoTargetStringParameterRunsWithTheName) {
	// hud's own compatibility rule, kept from before
	EXPECT_EQ("'the-action'", run(true, "s", nullptr));
}

TEST(TestActivationParameter, NoTargetOtherParameterDoesNotRun) {
	EXPECT_EQ("no activation", run(true, "(ss)", nullptr));
	EXPECT_EQ("no activation", run(true, "b", nullptr));
}

TEST(TestActivationParameter, TargetOfTheParameterTypeRunsWithIt) {
	EXPECT_EQ("'list'", run(true, "s", "'list'"));
	EXPECT_EQ("true", run(true, "b", "true"));
	EXPECT_EQ("int64 0", run(true, "x", "int64 0"));
	EXPECT_EQ("(uint32 80, uint32 24)",
			run(true, "(uu)", "(uint32 80, uint32 24)"));
	EXPECT_EQ("('window', 'current')",
			run(true, "(ss)", "('window', 'current')"));
}

TEST(TestActivationParameter, TargetWithoutParameterDoesNotRun) {
	// GTK leaves such an item insensitive
	EXPECT_EQ("no activation", run(true, nullptr, "'x'"));
}

TEST(TestActivationParameter, TargetOfAnotherTypeDoesNotRun) {
	EXPECT_EQ("no activation", run(true, "s", "true"));
	// without its annotation the (uu) target reads as (ii): refused, not
	// converted
	EXPECT_EQ("no activation", run(true, "(uu)", "(80, 24)"));
	EXPECT_EQ("no activation", run(true, "x", "0"));
}

TEST(TestActivationParameter, UnparsableTargetDoesNotRun) {
	EXPECT_EQ("no activation", run(true, "s", "('unterminated"));
}

}
