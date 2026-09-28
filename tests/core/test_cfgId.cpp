#include <gtest/gtest.h>
#include "cfgId.hpp"

using namespace cfg;

TEST(CfgIdTest, MakeIdAndExtractFields) {
    uint32_t idRaw = MakeId(Category::Raw, Subsystem::Network, 0x1234);
    EXPECT_EQ(GetCategory(idRaw), Category::Raw);
    EXPECT_EQ(GetSubsystem(idRaw), Subsystem::Network);
    EXPECT_EQ(GetItemId(idRaw), 0x1234);
    EXPECT_FALSE(IsReadOnly(idRaw));
    EXPECT_FALSE(IsVolatile(idRaw));

    uint32_t idSensitive = MakeId(Category::Sensitive, Subsystem::Security, 0x0001, Flag::ReadOnly);
    EXPECT_EQ(GetCategory(idSensitive), Category::Sensitive);
    EXPECT_EQ(GetSubsystem(idSensitive), Subsystem::Security);
    EXPECT_EQ(GetItemId(idSensitive), 0x0001);
    EXPECT_TRUE(IsReadOnly(idSensitive));
    EXPECT_FALSE(IsVolatile(idSensitive));

    uint32_t idSecure = MakeId(Category::Secure, Subsystem::System, 0xFFFF, Flag::Volatile);
    EXPECT_EQ(GetCategory(idSecure), Category::Secure);
    EXPECT_EQ(GetSubsystem(idSecure), Subsystem::System);
    EXPECT_EQ(GetItemId(idSecure), 0xFFFF);
    EXPECT_FALSE(IsReadOnly(idSecure));
    EXPECT_TRUE(IsVolatile(idSecure));
}

TEST(CfgIdTest, BoundaryValues) {
    uint32_t minItem = MakeId(Category::Raw, Subsystem::App, 0x0000);
    EXPECT_EQ(GetItemId(minItem), 0x0000);

    uint32_t maxItem = MakeId(Category::Raw, Subsystem::App, 0xFFFF);
    EXPECT_EQ(GetItemId(maxItem), 0xFFFF);
}

TEST(CfgIdTest, ValidationRules) {
    // Valid IDs
    EXPECT_TRUE(ValidateId(MakeId(Category::Raw, Subsystem::System, 1)));
    EXPECT_TRUE(ValidateId(MakeId(Category::Sensitive, Subsystem::Network, 2)));
    EXPECT_TRUE(ValidateId(MakeId(Category::Secure, Subsystem::Security, 3)));
    EXPECT_TRUE(ValidateId(MakeId(Category::Raw, Subsystem::App, 4, Flag::ReadOnly | Flag::Volatile)));

    // Invalid: No category flag
    uint32_t noCat = (0x01 << 16) | 0x0001; // Subsystem=1, Item=1, Flags=0
    EXPECT_FALSE(ValidateId(noCat));

    // Invalid: Conflicting category flags (Raw + Sensitive)
    uint32_t conflictingCat = ((static_cast<uint32_t>(Category::Raw) | static_cast<uint32_t>(Category::Sensitive)) << 24) |
                              (0x01 << 16) | 0x0001;
    EXPECT_FALSE(ValidateId(conflictingCat));

    // Invalid: Conflicting category flags (Sensitive + Secure)
    uint32_t conflictingCat2 = ((static_cast<uint32_t>(Category::Sensitive) | static_cast<uint32_t>(Category::Secure)) << 24) |
                               (0x01 << 16) | 0x0001;
    EXPECT_FALSE(ValidateId(conflictingCat2));

    // Invalid: Unknown Subsystem (e.g. 0x00 or 0x99)
    uint32_t zeroSubsys = (static_cast<uint32_t>(Category::Raw) << 24) | (0x00 << 16) | 0x0001;
    EXPECT_FALSE(ValidateId(zeroSubsys));

    uint32_t unknownSubsys = (static_cast<uint32_t>(Category::Raw) << 24) | (0x99 << 16) | 0x0001;
    EXPECT_FALSE(ValidateId(unknownSubsys));

    // Invalid: Unsupported high flag bits (e.g. 0x80)
    uint32_t badFlags = ((static_cast<uint32_t>(Category::Raw) | 0x80) << 24) | (0x01 << 16) | 0x0001;
    EXPECT_FALSE(ValidateId(badFlags));
}
